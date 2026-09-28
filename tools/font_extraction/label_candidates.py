"""Label existing cuts using OCR; never modify any candidate raster or contour."""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import shutil

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from rapidocr import RapidOCR
from rapidocr.utils.typings import LangRec, OCRVersion, ModelType
from fontTools.ttLib import TTFont


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--candidates", type=Path, required=True)
    p.add_argument("--previous", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    def read(path):
        return json.loads(path.read_text(encoding="utf-8"))
    def save(name, obj):
        (args.output / name).write_text(json.dumps(obj, ensure_ascii=False, indent=2), encoding="utf-8")
    def engine(lang, version):
        return RapidOCR(params={"Rec.lang_type":lang, "Rec.ocr_version":version,
                               "Rec.model_type":ModelType.MOBILE, "Global.use_det":False,
                               "Global.use_cls":False, "Global.log_level":"warning"})
    jp = engine(LangRec.JAPAN, OCRVersion.PPOCRV4)
    multi = engine(LangRec.CH, OCRVersion.PPOCRV5)
    en = engine(LangRec.EN, OCRVersion.PPOCRV4)
    def recognize(mask, model, factor):
        # Temporary recognition input only. Nothing here is saved as a glyph.
        image = cv2.copyMakeBorder(255-mask, 4, 4, 4, 4, cv2.BORDER_CONSTANT, value=255)
        image = cv2.resize(image, None, fx=factor, fy=factor, interpolation=cv2.INTER_CUBIC)
        result = model(cv2.cvtColor(image, cv2.COLOR_GRAY2BGR), use_det=False, use_cls=False)
        return {"text":result.txts[0].strip() if result.txts else "", "confidence":float(result.scores[0]) if result.txts else 0}
    candidates = read(args.candidates / "candidates.json")
    sources = {s["id"]:s for s in read(args.candidates / "sources.json")}
    context = collections.defaultdict(set)
    for row in read(args.previous / "lines.json"):
        context[(row["archive"], row["path"])].update(row["text"])
    evidence, accepted = [], collections.defaultdict(list)
    before_hashes = {i["id"]:hashlib.sha256((args.candidates/i["rgba_file"]).read_bytes()).hexdigest() for i in candidates}
    for i, item in enumerate(candidates):
        record = {"id":item["id"], "accepted":False}
        evidence.append(record)
        if item["width"] / item["line_height"] > 1.35:
            record["reason"] = "wide proposal may contain multiple characters"
            continue
        if all(o["mode"] == "alpha" for o in item["occurrences"]):
            record["reason"] = "outline/shadow-only candidate retained without mapping"
            continue
        mask = np.asarray(Image.open(args.candidates/item["mask_file"]).convert("L"))
        a, b = recognize(mask, jp, 3), recognize(mask, multi, 3)
        votes = [("japanese-v4", a), ("multilingual-v5", b)]
        if any(len(r["text"]) == 1 and r["text"].isascii() for r in (a,b)):
            votes.append(("english-v4", recognize(mask, en, 4)))
        record["readings"] = dict(votes)
        counts = collections.Counter(r["text"] for _, r in votes if len(r["text"]) == 1 and r["confidence"] >= .95)
        if counts:
            char, count = counts.most_common(1)[0]
            has_context = any(char in context[(sources[o["source"]]["archive"], sources[o["source"]]["path"])] for o in item["occurrences"])
            record.update(proposed_character=char, agreeing_models=count, prior_line_context=has_context)
            if count >= 2 and has_context:
                record["accepted"] = True
                accepted[char].append(item)
        if not record["accepted"]:
            record["reason"] = "requires two confident models plus prior source-text evidence"
        if i % 30 == 0:
            print(f"recognition {i}/{len(candidates)}; labels {len(accepted)}", flush=True)
            save("recognition-progress.json", evidence)
    original = TTFont(args.previous / "kinoko-raster-experimental.ttf")
    archived = TTFont(args.candidates / "kinoko-candidate-archive.ttf")
    original_cmap = dict(original.getBestCmap())
    order = list(original.getGlyphOrder())
    added = {}
    (args.output / "new-glyphs").mkdir()
    for char, alternatives in sorted(accepted.items()):
        if ord(char) in original_cmap:
            continue
        item = max(alternatives, key=lambda x:(x["line_height"],len(x["occurrences"])))
        codepoint = int(item["private_codepoint"][2:], 16)
        source_name = archived.getBestCmap()[codepoint]
        name = f"uni{ord(char):04X}"
        # Copy the already-exported outline verbatim; OCR never draws contours.
        original["glyf"][name] = archived["glyf"][source_name]
        original["hmtx"][name] = archived["hmtx"][source_name]
        order.append(name)
        for table in original["cmap"].tables:
            if table.isUnicode():
                table.cmap[ord(char)] = name
        added[char] = dict(candidate=item["id"], rgba_file=item["rgba_file"],
                           occurrences=item["occurrences"], glyph_name=name)
        shutil.copy2(args.candidates/item["rgba_file"], args.output/"new-glyphs"/f"U{ord(char):04X}.png")
    original.setGlyphOrder(order)
    for nid, value in {1:"Kinoko Raster Experimental", 3:"KinokoRasterExperimental-11",4:"Kinoko Raster Experimental 11",5:"Version 0.11",6:"KinokoRasterExperimental-11"}.items():
        original["name"].setName(value,nid,3,1,0x409)
        original["name"].setName(value,nid,1,0,0)
    target = args.output/"kinoko-raster-experimental.ttf"
    original.save(target)
    reopened = TTFont(target)
    assert set(original_cmap).issubset(reopened.getBestCmap())
    after_hashes = {i["id"]:hashlib.sha256((args.candidates/i["rgba_file"]).read_bytes()).hexdigest() for i in candidates}
    assert before_hashes == after_hashes
    font = ImageFont.truetype(str(target),32)
    chars = sorted(c for c in reopened.getBestCmap() if c != 32)
    image = Image.new("RGB",(800,math_ceil(len(chars),16)*70+20),"white")
    draw = ImageDraw.Draw(image)
    for i, cp in enumerate(chars):
        x,y = i%16*50,i//16*70
        draw.text((x+3,y),chr(cp),font=font,fill="black")
        draw.text((x+2,y+45),f"{cp:04X}",fill="black")
    image.save(args.output/"exported-font-preview.png")
    save("recognition.json",evidence)
    save("added-glyphs.json",added)
    stats = dict(characters=len(chars),previous_characters=len(original_cmap)-1,added_characters=len(added),
                 complete=False,ocr_changes_pixels=False,raw_candidate_hashes_unchanged=True,
                 cmap_reopened_and_rendered=True,sha256=hashlib.sha256(target.read_bytes()).hexdigest())
    save("validation.json",stats)
    print(json.dumps(stats),flush=True)


def math_ceil(n, divisor):
    return (n+divisor-1)//divisor


if __name__ == "__main__":
    main()
