"""Extract image candidates without OCR; retain source pixels and uncertain cuts.

Requires numpy, opencv-python-headless, Pillow and fonttools. The previous pass
supplies source-style decisions and an optional already-mapped experimental font,
not segmentation coordinates or character counts. Candidate count is NOT a count
of recovered characters. This script never installs a font or changes game data.
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import math
import struct

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
from fontTools.pens.ttGlyphPen import TTGlyphPen


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def runs(vector):
    edges = np.flatnonzero(np.diff(np.r_[False, vector, False].astype(int)))
    return [(int(a), int(b)) for a, b in zip(edges[::2], edges[1::2])]


def decode(entry, reference):
    with (reference / entry["archive"]).open("rb") as stream:
        stream.seek(entry["offset"])
        data = stream.read(entry["size"])
    key = ((entry["offset"] >> 1) | 0x23) & 255
    data = data.translate(bytes(x ^ key for x in range(256)))
    depth, width, height, stride, packed = struct.unpack_from("<BIIII", data)
    if depth not in (16, 24, 32):
        raise ValueError(f"unsupported depth {depth}")
    dtype = "<u2" if depth == 16 else "<u4"
    if packed:
        pairs = np.frombuffer(data[17:17 + packed], dtype=dtype).reshape(-1, 2)
        if int(pairs[:, 0].sum()) != width * height:
            raise ValueError("unexpected RLE pixel count")
        pixels = np.repeat(pairs[:, 1], pairs[:, 0].astype(int)).reshape(height, width)
    else:
        pixels = np.frombuffer(data[17:], dtype=dtype).reshape(height, stride)[:, :width]
    if depth == 16:
        rgb = np.stack([(pixels >> shift) & 31 for shift in (10, 5, 0)], axis=2)
        rgb = np.rint(rgb.astype(float) * 255 / 31).astype("uint8")
        alpha = np.where(pixels & 32768, 255, 0).astype("uint8")
    else:
        rgb = np.stack([(pixels >> shift) & 255 for shift in (16, 8, 0)], axis=2).astype("uint8")
        alpha = (pixels >> 24).astype("uint8") if depth == 32 else np.full((height, width), 255, "uint8")
    return np.dstack([rgb, alpha]), depth


def column_groups(mask):
    """Blank columns define proposals; attach only tiny, close fragments.

    This deliberately does not assume that a connected component is a character.
    Ambiguous proposals remain available together with an uncut parent row.
    """
    spans = runs(mask.any(0))
    height = mask.shape[0]
    merged = []
    while spans:
        left, right = spans.pop(0)
        if right - left < height * .24:
            before = left - merged[-1][1] if merged else math.inf
            after = spans[0][0] - right if spans else math.inf
            if min(before, after) <= max(1, height * .18):
                if before <= after:
                    merged[-1] = (merged[-1][0], right)
                    continue
                spans[0] = (left, spans[0][1])
                continue
        merged.append((left, right))
    return merged


def make_sheet(items, output, title):
    # Paginate to avoid unusably tall sheets. Labels are stable candidate IDs.
    for page in range(0, len(items), 120):
        subset = items[page:page + 120]
        sheet = Image.new("RGB", (1000, 40 + math.ceil(len(subset) / 10) * 92), "#282c34")
        draw = ImageDraw.Draw(sheet)
        draw.text((12, 10), title, fill="white")
        for i, item in enumerate(subset):
            source = Image.open(output / item["mask_file"]).convert("L")
            source.thumbnail((82, 60), Image.Resampling.NEAREST)
            x, y = i % 10 * 100 + 8, i // 10 * 92 + 36
            sheet.paste(Image.new("RGB", source.size, "white"), (x, y), source)
            draw.text((x, y + 63), item["id"], fill="#bfc8d5")
        sheet.save(output / f"candidates-{page // 120 + 1:02d}.png")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--previous", type=Path, required=True)
    parser.add_argument("--entries", type=Path, required=True)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    # Each run must use a fresh directory. Never overwrite earlier artifacts.
    args.output.mkdir(parents=True, exist_ok=False)
    for name in ("sources", "rows", "candidates", "quarantine"):
        (args.output / name).mkdir()
    checks = read_json(args.previous / "source-style-checks.json")
    selected = {(r["archive"], r["path"]) for r in checks if r["accepted"]}
    considered = {(r["archive"], r["path"]) for r in checks}
    inventory, rows, candidates = [], [], {}
    for entry in read_json(args.entries):
        key = (entry["archive"], entry["path"])
        if key not in considered:
            continue
        sid = hashlib.sha256((key[0] + ":" + key[1]).encode()).hexdigest()[:12]
        accepted = key in selected
        record = dict(entry, id=sid, style_accepted=accepted)
        inventory.append(record)
        try:
            rgba, depth = decode(entry, args.reference)
        except Exception as exc:
            record["error"] = str(exc)
            continue
        relative = f"{'sources' if accepted else 'quarantine'}/{sid}.png"
        Image.fromarray(rgba).save(args.output / relative)
        # Round-trip check of original pixels, including alpha, not just the mask.
        assert np.array_equal(np.asarray(Image.open(args.output / relative)), rgba)
        record.update(file=relative, depth=depth, width=rgba.shape[1], height=rgba.shape[0],
                      alpha_zero_fraction=float((rgba[:, :, 3] == 0).mean()),
                      rgba_sha256=hashlib.sha256(rgba.tobytes()).hexdigest())
        if not accepted:
            record["status"] = "style-unconfirmed; preserved but excluded from font"
            continue
        alpha = rgba[:, :, 3].astype(float) / 255
        # Alpha is retained losslessly. For segmentation, also try the bright face
        # when a black shadow/outline bridges transparent-background characters.
        variants = {"alpha": rgba[:, :, 3] > 16}
        for mode, face in (("color", rgba[:, :, :3].max(2)), ("white", rgba[:, :, :3].min(2))):
            mask = face * alpha > 160
            if not any(np.array_equal(mask, v) for v in variants.values()):
                variants[mode] = mask
        record["variants"] = list(variants)
        for mode, mask in variants.items():
            for ri, (top, bottom) in enumerate(runs(mask.any(1))):
                occupied = np.flatnonzero(mask[top:bottom].any(0))
                if not len(occupied):
                    continue
                left, right = int(occupied[0]), int(occupied[-1]) + 1
                row_id = f"{sid}-{mode}-{ri:03d}"
                row_file = f"rows/{row_id}.png"
                Image.fromarray(rgba[top:bottom, left:right]).save(args.output / row_file)
                row = dict(id=row_id, source=sid, mode=mode, box=[left, top, right, bottom], file=row_file)
                rows.append(row)
                if bottom - top < 8 or bottom - top > 100 or (mode == "alpha" and mask.mean() > .9):
                    row["status"] = "preserved; unsuitable text-row geometry"
                    continue
                crop = mask[top:bottom, left:right]
                groups = column_groups(crop)
                # Retain alternative unions for characters with disconnected
                # vertical strokes. These are hypotheses, not forced merges;
                # downstream OCR may label them without changing their pixels.
                base_groups = list(groups)
                for first in range(len(base_groups)):
                    for last in range(first+1, min(first+4, len(base_groups))):
                        a, b = base_groups[first][0], base_groups[last][1]
                        ratio = (b-a)/(bottom-top)
                        if .55 <= ratio <= 1.30:
                            groups.append((a, b))
                row["proposals"] = len(groups)
                for start, end in groups:
                    ys, xs = np.where(crop[:, start:end])
                    x0, x1 = left + start + int(xs.min()), left + start + int(xs.max()) + 1
                    y0, y1 = top + int(ys.min()), top + int(ys.max()) + 1
                    face = mask[y0:y1, x0:x1].astype("uint8") * 255
                    digest = hashlib.sha256(struct.pack("<II", face.shape[1], face.shape[0]) + face.tobytes()).hexdigest()
                    occurrence = dict(source=sid, row=row_id, mode=mode, box=[x0, y0, x1, y1])
                    if digest in candidates:
                        candidates[digest]["occurrences"].append(occurrence)
                        continue
                    cid = f"g{len(candidates):04d}"
                    ratio = face.shape[1] / (bottom - top)
                    flags = []
                    if ratio > 1.35:
                        flags.append("possible joined characters or decoration")
                    if ratio < .24:
                        flags.append("possible detached stroke or punctuation")
                    if (start, end) not in base_groups:
                        flags.append("alternative union of disconnected column groups")
                    if mode == "alpha":
                        flags.append("alpha includes outline/shadow")
                    item = dict(id=cid, digest=digest, width=face.shape[1], height=face.shape[0],
                                line_height=bottom-top, y_offset=y0-top, flags=flags,
                                mask_file=f"candidates/{cid}-mask.png", rgba_file=f"candidates/{cid}.png",
                                occurrences=[occurrence])
                    Image.fromarray(face).save(args.output / item["mask_file"])
                    Image.fromarray(rgba[y0:y1, x0:x1]).save(args.output / item["rgba_file"])
                    candidates[digest] = item
        record["status"] = "preserved and segmented without OCR"
    items = list(candidates.values())
    # Keep the existing Unicode mapping unchanged. New unlabelled shapes go in
    # the PUA only: never invent a Unicode character because a cut looks plausible.
    font = TTFont(args.previous / "kinoko-raster-experimental.ttf")
    original_cmap = dict(font.getBestCmap())
    order = list(font.getGlyphOrder())
    mapping = {}
    for i, item in enumerate(items):
        codepoint = 0xE000 + i if i < 0x1900 else 0xF0000 + i - 0x1900
        if codepoint > 0xFFFFD:
            raise ValueError("PUA capacity exceeded")
        name = f"candidate{i:05d}"
        item["private_codepoint"] = f"U+{codepoint:04X}"
        mapping[codepoint] = name
        mask = np.asarray(Image.open(args.output / item["mask_file"]))
        mask = cv2.resize(mask, None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST)
        contours, _ = cv2.findContours(mask, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)
        scale = 760 / item["line_height"]
        pen = TTGlyphPen(None)
        for contour in contours:
            points = contour[:, 0, :]
            if len(points) < 3:
                continue
            coords = [(round(float(x)/4*scale+35), round(760-(float(y)/4+item["y_offset"])*scale)) for x, y in points]
            pen.moveTo(coords[0])
            for point in coords[1:]:
                pen.lineTo(point)
            pen.closePath()
        font["glyf"][name] = pen.glyph()
        font["hmtx"][name] = (max(100, round(item["width"]*scale)+70), 0)
        order.append(name)
    font.setGlyphOrder(order)
    for table in font["cmap"].tables:
        if table.isUnicode():
            table.cmap.update({cp: name for cp, name in mapping.items() if cp <= 0xFFFF or table.format in (12, 13)})
    if any(cp > 0xFFFF for cp in mapping):
        from fontTools.ttLib.tables._c_m_a_p import CmapSubtable
        table = CmapSubtable.newSubtable(12)
        table.platformID, table.platEncID, table.language = 3, 10, 0
        table.cmap = {**original_cmap, **mapping}
        font["cmap"].tables.append(table)
    for nid, value in {1:"Kinoko Candidate Archive", 3:"KinokoCandidateArchive-08", 4:"Kinoko Candidate Archive 08", 5:"Version 0.08", 6:"KinokoCandidateArchive-08"}.items():
        font["name"].setName(value, nid, 3, 1, 0x409)
        font["name"].setName(value, nid, 1, 0, 0)
    target = args.output / "kinoko-candidate-archive.ttf"
    font.save(target)
    reopened = TTFont(target)
    assert all(reopened.getBestCmap().get(cp) == name for cp, name in original_cmap.items())
    assert all(reopened.getBestCmap().get(cp) == name for cp, name in mapping.items())
    # Render the actual TTF, separate from the source-mask contact sheets.
    preview = Image.new("RGB", (1000, 480), "white")
    draw = ImageDraw.Draw(preview)
    rendered_font = ImageFont.truetype(str(target), 30)
    for i, cp in enumerate(list(mapping)[:100]):
        draw.text(((i%10)*100, (i//10)*48), chr(cp), font=rendered_font, fill="black")
    preview.save(args.output / "font-pua-preview.png")
    make_sheet(items, args.output, "Automatic cut candidates (not verified characters)")
    stats = dict(unicode_characters_excluding_space=len(original_cmap)-1,
                 unique_cut_candidates=len(items), candidate_occurrences=sum(len(i["occurrences"]) for i in items),
                 accepted_sources=sum(r["style_accepted"] and "error" not in r for r in inventory),
                 quarantined_sources=sum(not r["style_accepted"] and "error" not in r for r in inventory),
                 preserved_rows=len(rows), decode_errors=[r for r in inventory if "error" in r],
                 ocr_used_for_segmentation=False, style_selection_inherited_from_previous_pass=True,
                 manual_labels=False, manual_cuts=False, complete=False,
                 source_png_pixel_roundtrip_verified=True, font_cmap_and_render_verified=True,
                 sha256=hashlib.sha256(target.read_bytes()).hexdigest())
    write_json(args.output / "sources.json", inventory)
    write_json(args.output / "rows.json", rows)
    write_json(args.output / "candidates.json", items)
    write_json(args.output / "validation.json", stats)
    print(json.dumps(stats, ensure_ascii=True, indent=2))


if __name__ == "__main__":
    main()
