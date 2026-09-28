"""Collect the incremental font, original RGBA glyphs and unresolved evidence."""
from pathlib import Path
import argparse, collections, hashlib, json, shutil, re, math, unicodedata
import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
from fontTools.pens.ttGlyphPen import TTGlyphPen
from export_unlabeled import decode


def read(path):return json.loads(path.read_text(encoding="utf-8"))
def write(path,obj):path.write_text(json.dumps(obj,ensure_ascii=False,indent=2),encoding="utf-8")


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--latest",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--passes",nargs="+",type=int,default=[16,17,18,19,20,21,22])
    p.add_argument("--reference",type=Path,default=Path("C:/WorkSpace/6kinoko"))
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    for folder in ("glyphs","sources","unresolved"):(args.output/folder).mkdir()
    inventory=read(Path("docs/font-resource-investigation/entries.json"))
    entries={(e["archive"],e["path"]):e for e in inventory}
    old=read(Path("analysis/font-extraction-07/manifest.json"));records={};cache={}
    def source_pixels(archive,path):
        key=(archive,path)
        if key not in cache:cache[key]=decode(entries[key],args.reference)[0]
        return cache[key]
    for ch,g in old["glyphs"].items():
        info=g["source"];left,top,_,_=info["box"];l,r=g["cell"];a,b,c,d=g["bbox"]
        box=[left+l+a,top+b,left+l+c,top+d]
        records[ch]=dict(archive=info["archive"],path=info["path"],box=box,pass_number=7,
                        mask_file=str(Path("analysis/font-extraction-07/glyphs")/f"U{ord(ch):04X}.png"),evidence=g)
    cut_sources={s["id"]:s for s in read(Path("analysis/font-extraction-10/sources.json"))}
    cut_candidates={s["id"]:s for s in read(Path("analysis/font-extraction-10/candidates.json"))}
    for ch,g in read(Path("analysis/font-extraction-11/added-glyphs.json")).items():
        candidate=cut_candidates[g["candidate"]];occ=candidate["occurrences"][0];src=cut_sources[occ["source"]]
        records[ch]=dict(archive=src["archive"],path=src["path"],box=occ["box"],pass_number=11,
                        mask_file=str(Path("analysis/font-extraction-10")/candidate["mask_file"]),evidence=g)
    source_index={s["id"]:s for s in cut_sources.values()};styles={};all_samples=[];all_lines=[]
    for batch in args.passes:
        root=Path(f"analysis/font-extraction-{batch:02d}")
        samples={s["id"]:s for s in read(root/"samples.json")}
        for s in read(root/"style-checks.json"):styles[s["source"]]=s
        for s in samples.values():all_samples.append((root,s))
        all_lines.extend(read(root/"lines.json"))
        for ch,ident in read(root/"added-glyphs.json").items():
            s=samples[ident];src=source_index[s["source"]]
            records[ch]=dict(archive=src["archive"],path=src["path"],box=s["box"],pass_number=batch,
                            mask_file=str(root/s["mask_file"]),evidence=s)
    audit=read(Path("analysis/font-resource-audit-15/inventory.json"))
    raw_text={(e["archive"],e["path"]):e.get("detections",[]) for e in audit}
    punctuation=collections.defaultdict(list)
    for root,s in all_samples:
        ch=s["char"]
        if ch in records or not unicodedata.category(ch).startswith("P") or not s["style_accepted"]:continue
        src=source_index.get(s["source"])
        if not src:continue
        center=np.array([(s["box"][0]+s["box"][2])/2,(s["box"][1]+s["box"][3])/2])
        direct=[]
        for d in raw_text.get((src["archive"],src["path"]),[]):
            b=np.asarray(d["box"])
            if ch in unicodedata.normalize("NFKC",d["text"]) and d["confidence"]>=.95 and (center>=b.min(0)-2).all() and (center<=b.max(0)+2).all():direct.append(d)
        contexts=[l for l in all_lines if l["source"]==s["source"] and l["mode"]==s["mode"] and l["text"]==s["line_text"] and sum(t==l["text"] and c>=.94 for t,c in l["readings"])>=2]
        if direct and contexts:punctuation[ch].append((root,s,direct,contexts))
    punctuation_added={}
    for ch,options in punctuation.items():
        root,s,direct,contexts=max(options,key=lambda v:(v[1]["line_height"],v[1]["line_confidence"]))
        src=source_index[s["source"]]
        records[ch]=dict(archive=src["archive"],path=src["path"],box=s["box"],pass_number=int(root.name.split("-")[-1]),mask_file=str(root/s["mask_file"]),evidence=s,punctuation_context_evidence=dict(raw=direct,masked_lines=contexts))
        punctuation_added[ch]=s
    # Dark ink is the face of illustrated help text, but on credits tiles it
    # is an outline surrounding a white face. Prefer a verified face sample to
    # avoid exporting a hollow outline as the glyph itself.
    replacements=dict(punctuation_added)
    for ch,r in records.items():
        current=r["evidence"]
        if r["pass_number"]<=11 or current.get("mode")!="dark-ink" or re.search(r"/pause/gp\d",r["path"].lower()):continue
        alternatives=[(root,s) for root,s in all_samples if s["char"]==ch and s["source_path"]==r["path"] and s["mode"] in ("white","white-core","color","yellow") and s.get("verification",{}).get("single_models",0)>=1]
        if not alternatives:continue
        root,s=max(alternatives,key=lambda pair:(pair[1]["verification"]["single_models"],pair[1]["line_height"],pair[1]["line_confidence"]))
        r.update(box=s["box"],pass_number=int(root.name.split("-")[-1]),mask_file=str(root/s["mask_file"]),evidence=s)
        replacements[ch]=s
    target=args.output/"kinoko-raster-experimental.ttf"
    font=TTFont(args.latest/target.name)
    order=list(font.getGlyphOrder())
    for ch,s in replacements.items():
        r=records[ch];mask=np.asarray(Image.open(r["mask_file"]).convert("L"));mask=cv2.resize((mask>127).astype("uint8")*255,None,fx=4,fy=4,interpolation=cv2.INTER_NEAREST)
        contours,_=cv2.findContours(mask,cv2.RETR_LIST,cv2.CHAIN_APPROX_SIMPLE);pen=TTGlyphPen(None);scale=760/s["line_height"]
        for contour in contours:
            points=contour[:,0,:]
            if len(points)<3:continue
            coords=[(round(float(x)/4*scale+35),round(760-(float(y)/4+s["y_offset"])*scale)) for x,y in points]
            pen.moveTo(coords[0])
            for point in coords[1:]:pen.lineTo(point)
            pen.closePath()
        name=font.getBestCmap().get(ord(ch))
        if name is None:
            name=f"uni{ord(ch):04X}";order.append(name)
            for table in font["cmap"].tables:
                if table.isUnicode():table.cmap[ord(ch)]=name
        font["glyf"][name]=pen.glyph();font["hmtx"][name]=(max(100,round(s["width"]*scale)+70),0)
    font.setGlyphOrder(order)
    font.save(target)
    cmap=TTFont(target).getBestCmap();assert set(map(ord,records))==set(cmap)-{32}
    preview=Image.new("RGB",(1000,math.ceil(len(records)/20)*70+10),"white");draw=ImageDraw.Draw(preview);renderfont=ImageFont.truetype(str(target),32)
    for i,cp in enumerate(sorted(set(cmap)-{32})):
        x,y=i%20*50,i//20*70;draw.text((x+3,y),chr(cp),font=renderfont,fill="black");draw.text((x+2,y+45),f"{cp:04X}",fill="black")
    preview.save(args.output/"exported-font-preview.png")
    for ch,r in records.items():
        rgba=source_pixels(r["archive"],r["path"]);x0,y0,x1,y1=r["box"]
        crop=rgba[y0:y1,x0:x1];filename=f"glyphs/U{ord(ch):04X}.png"
        Image.fromarray(crop).save(args.output/filename)
        assert np.array_equal(crop,np.asarray(Image.open(args.output/filename)))
        shutil.copy2(r["mask_file"],args.output/"glyphs"/f"U{ord(ch):04X}-mask.png")
        r["rgba_file"]=filename;r["rgba_sha256"]=hashlib.sha256(crop.tobytes()).hexdigest()
    accepted={sid for sid,s in styles.items() if s["accepted"]}
    for sid in sorted(accepted):
        if sid not in source_index:continue
        src=source_index[sid];rgba=source_pixels(src["archive"],src["path"])
        Image.fromarray(rgba).save(args.output/"sources"/f"{sid}.png")
    unresolved=[];seen=set()
    for root,s in all_samples:
        if s["source"] not in accepted or ord(s["char"]) in cmap:continue
        signature=(s["char"],s["source"],tuple(s["box"]),s["mode"])
        if signature in seen:continue
        seen.add(signature)
        name=f"u{len(unresolved):04d}.png"
        shutil.copy2(root/s["rgba_file"],args.output/"unresolved"/name)
        unresolved.append(dict(proposed_char=s["char"],image=f"unresolved/{name}",sample=s,pass_directory=str(root)))
    missing=collections.defaultdict(list)
    for line in all_lines:
        if line["source"] not in accepted or line["agreements"]<2 or line["confidence"]<.94:continue
        for ch in set(line["text"]):
            if ord(ch) not in cmap:
                missing[ch].append(dict(source=line["source"],path=line["path"],text=line["text"],box=line["box"]))
    for ch in missing:
        unique={json.dumps(v,sort_keys=True):v for v in missing[ch]};missing[ch]=list(unique.values())
    stats=dict(characters=len(records),added_since_previous_user_delivery=len(records)-153,
               all_cv2_entries_audited=len(audit),decode_errors=sum("error" in e for e in audit),
               retained_target_source_textures=len(accepted),unresolved_crop_proposals=len(unresolved),
               unresolved_consensus_character_proposals=len(missing),complete=False,
               all_exported_rgba_glyphs_pixel_verified=True,font_cmap_matches_provenance=True,
               manual_labels=False,manual_cuts=False,sha256=hashlib.sha256(target.read_bytes()).hexdigest())
    stats["outline_samples_replaced_with_verified_faces"]=[c for c in replacements if c not in punctuation_added]
    stats["punctuation_from_raw_and_masked_line_context"]=list(punctuation_added)
    write(args.output/"glyph-provenance.json",records)
    write(args.output/"source-style-checks.json",list(styles.values()))
    write(args.output/"unresolved-candidates.json",unresolved)
    write(args.output/"missing-consensus-characters.json",dict(missing))
    write(args.output/"validation.json",stats)
    text=f"""# 目标字体字形收集：本批交付

当前 TTF 包含 **{len(records)} 个字符及空格**，比上一批 153 个增加 {len(records)-153} 个。
**尚不能确认已补齐所有原版字形。** 未确认项保留在 unresolved/ 中，不强行指定 Unicode。

## 已完成的范围

- 三个原版 DAT 的全部 {len(audit)} 个 CV2 均已解码并执行文本检测，解码错误为 0。
- 菜单、关卡名、说明面板、设置文字和两张字幕方格图纳入自动字体归属比较。
- 每个已映射字符都保存原始 RGBA 裁片、工作掩码、来源位置和识别证据。
- 字形直接来自图片；OCR 只处理临时输入，提供定位和字符映射，不生成或重绘轮廓。
- 整行两模型一致的结果可用于复核单字容易混淆的字符；该证据仍可能出错，详情见逐字记录。
- 原始 RGBA 裁片逐一与解码源像素比对，字体 cmap 与来源记录逐一核对。

## 剩余限制

全量文本检测不代表检测器不会漏字。现有记录仍有 {len(missing)} 种未映射的整行 OCR 字符提议；
其中可能包含误识别、图标和标点，不能把该数字当作真实缺字数量。
保留了 {len(unresolved)} 个未映射裁片提议，也可能包含错切、同一字形的不同尺寸或其他非字符内容。
已有旧版映射随增量字体保留，并未全部重新认证。原始字体名称尚未确认。
TTF 仍使用栅格边界轮廓，未进行平滑；原始 PNG 是保留抗锯齿细节的依据。

## 文件

- kinoko-raster-experimental.ttf：当前可按正常字符输入的字体子集。
- exported-font-preview.png：直接使用该 TTF 渲染的预览。
- glyphs/Uxxxx.png：原始 RGBA 字形裁片；同名 -mask.png 是工作掩码。
- glyph-provenance.json：每字的源档案、图片路径、坐标和识别证据。
- sources/：本轮已接受字体归属的完整素材，保留未成功切分的内容。
- unresolved/、unresolved-candidates.json、missing-consensus-characters.json：未确认项。
- validation.json：统计与文件 SHA256。

未修改游戏、未运行游戏、未安装字体。全部旧批次和日志保留。
"""
    (args.output/"README.md").write_text(text,encoding="utf-8")
    print(json.dumps(stats,ensure_ascii=True,indent=2))


if __name__=="__main__":main()
