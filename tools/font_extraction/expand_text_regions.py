"""Extract additional font samples from detected text, grids and projected rows.

Recognition operates on temporary images. All saved RGBA crops come directly
from decoded game pixels. Font-family checks and uncertain samples are logged.
"""
from pathlib import Path
import argparse, collections, hashlib, json, math, unicodedata
import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from rapidocr import RapidOCR
from rapidocr.utils.typings import LangRec, OCRVersion, ModelType
from fontTools.ttLib import TTFont
from fontTools.pens.ttGlyphPen import TTGlyphPen
from export_unlabeled import decode, runs, column_groups


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def normalize(mask):
    ys, xs = np.where(mask > 127)
    if not len(xs):
        return np.zeros((48, 48), "uint8")
    mask = mask[ys.min():ys.max()+1, xs.min():xs.max()+1]
    return (cv2.resize(mask, (48, 48)) > 127).astype("uint8")


def similarity(a, b):
    # At small source sizes a half-pixel boundary uncertainty is several pixels
    # in the normalized comparison. This changes comparison only, not the crop.
    radius = min(2, max(1, round(24/min(a.shape[0],b.shape[0]))))
    a, b = normalize(a), normalize(b)
    best = 0.0
    kernel=np.ones((radius+1,radius+1),"uint8")
    for variant in (a, cv2.dilate(a,kernel), cv2.erode(a,kernel)):
        for dx, dy in ((0,0),(-radius,0),(radius,0),(0,-radius),(0,radius)):
            shifted = np.roll(variant, (dy, dx), (0,1))
            best = max(best, float((shifted & b).sum()/max(1,(shifted | b).sum())))
    return best


def segment(mask, text):
    spans = runs(mask.any(0))
    if text.isascii() and len(spans) == len(text):
        return [0]+[(spans[i-1][1]+spans[i][0])//2 for i in range(1,len(spans))]+[mask.shape[1]]
    n, width = len(text), mask.shape[1]
    if not n:
        return None
    gaps = runs(~mask.any(0))
    points = sorted(set([0,width]+[(a+b)//2 for a,b in gaps if a>0 and b<width]))
    target = width/n
    dp = {(0,0):(0,[])}
    for k in range(1,n+1):
        for j in range(1,len(points)):
            best = None
            for i in range(j):
                prior = dp.get((k-1,i))
                w = points[j]-points[i]
                if prior is None or w < target*.22 or w > target*1.8:
                    continue
                score = prior[0] + ((w-target)/target)**2
                if best is None or score < best[0]:
                    best = (score, prior[1]+[points[j]])
            if best:
                dp[k,j] = best
    answer = dp.get((n,len(points)-1))
    return [0]+answer[1] if answer else None


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--previous",type=Path,default=Path("analysis/font-extraction-07"))
    p.add_argument("--base-font",type=Path,default=Path("analysis/font-extraction-11/kinoko-raster-experimental.ttf"))
    p.add_argument("--inventory",type=Path,default=Path("analysis/font-extraction-10/sources.json"))
    p.add_argument("--reference",type=Path,default=Path("C:/WorkSpace/6kinoko"))
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--server-recognizer",action="store_true",help="Use PP-OCRv5 server for multilingual cross-checks")
    args = p.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    for folder in ("sources","samples","glyphs"):
        (args.output/folder).mkdir()
    def engine(lang, version):
        size=ModelType.SERVER if args.server_recognizer and lang==LangRec.CH else ModelType.MOBILE
        return RapidOCR(params={"Rec.lang_type":lang,"Rec.ocr_version":version,"Rec.model_type":size,"Global.use_cls":False,"Global.log_level":"warning"})
    jp, mix, en = engine(LangRec.JAPAN,OCRVersion.PPOCRV4), engine(LangRec.CH,OCRVersion.PPOCRV5), engine(LangRec.EN,OCRVersion.PPOCRV4)
    cache = {}
    def recognize(mask, model, scale=3):
        key=(hashlib.sha256(mask.tobytes()).hexdigest(),mask.shape,id(model),scale)
        if key in cache:
            return cache[key]
        temporary=cv2.copyMakeBorder(255-mask,4,4,4,4,cv2.BORDER_CONSTANT,value=255)
        temporary=cv2.resize(temporary,None,fx=scale,fy=scale,interpolation=cv2.INTER_CUBIC)
        res=model(cv2.cvtColor(temporary,cv2.COLOR_GRAY2BGR),use_det=False,use_cls=False)
        text=unicodedata.normalize("NFKC",res.txts[0]).replace(" ","") if res.txts else ""
        cache[key]=(text,float(res.scores[0]) if res.txts else 0)
        return cache[key]
    anchors={ch:np.asarray(Image.open(args.previous/"glyphs"/f"U{ord(ch):04X}.png").convert("L")) for ch in read(args.previous/"manifest.json")["glyphs"]}
    all_samples=[]; line_audit=[]; style_audit=[]; inventory=read(args.inventory)
    for si, entry in enumerate(inventory):
        rgba,_=decode(entry,args.reference)
        sid=entry["id"]
        Image.fromarray(rgba).save(args.output/"sources"/f"{sid}.png")
        rgb=rgba[:,:,:3]; alpha=rgba[:,:,3].astype(float)/255
        composite=(rgb.astype(float)*alpha[:,:,None]).astype("uint8")
        temp=cv2.resize(composite,None,fx=3,fy=3,interpolation=cv2.INTER_CUBIC)
        result=jp(cv2.cvtColor(temp,cv2.COLOR_RGB2BGR),use_det=True,use_cls=False)
        regions=[]
        if result.boxes is not None:
            for box, text, confidence in zip(result.boxes,result.txts,result.scores):
                if confidence<.65:
                    continue
                box=np.asarray(box)/3
                x0,y0=np.floor(box.min(0)-1).astype(int);x1,y1=np.ceil(box.max(0)+1).astype(int)
                regions.append((max(0,int(x0)),max(0,int(y0)),min(rgba.shape[1],int(x1)),min(rgba.shape[0],int(y1)),"detected",text))
        masks={"white":(rgb.min(2)*alpha>160).astype("uint8")*255,"color":(rgb.max(2)*alpha>160).astype("uint8")*255}
        if (rgba[:,:,3]==0).mean()>.1 or (rgb.max(2)<8).mean()>.5:
            masks["white"]=(rgb.min(2)*alpha).astype("uint8")
        yellow=(np.minimum(rgb[:,:,0],rgb[:,:,1])>160)&(rgb[:,:,2].astype(float)<np.minimum(rgb[:,:,0],rgb[:,:,1])*.85)&(alpha>.5)
        if yellow.sum()>20:masks["yellow"]=yellow.astype("uint8")*255
        if "/staff" in entry["path"].lower() and rgba.shape[0]>=128:
            masks["white-core"]=(rgb.min(2)*alpha>208).astype("uint8")*255
        # Find dominant dark ink from detected text, rather than assuming that
        # opaque illustrated panels have transparent backgrounds.
        ink_pixels=[]
        for x0,y0,x1,y1,_,_ in regions:
            cut=rgb[y0:y1,x0:x1]; dark=(cut.max(2)<110)&(rgba[y0:y1,x0:x1,3]>128)
            ink_pixels.extend(map(tuple,cut[dark].tolist()))
        if ink_pixels:
            ink,count=collections.Counter(ink_pixels).most_common(1)[0]
            if count>=20:
                distance=np.linalg.norm(rgb.astype(float)-np.array(ink),axis=2)
                masks["dark-ink"]=(np.clip(1-distance/110,0,1)*255*alpha).astype("uint8")
        own=[]; seen=set()
        for mode,face in masks.items():
            proposals=list(regions)
            if mode!="dark-ink":
                row_runs=[]
                for a,b in runs((face>127).any(1)):
                    if row_runs and a-row_runs[-1][1]<=2:
                        row_runs[-1]=(row_runs[-1][0],b)
                    else:row_runs.append((a,b))
                for y0,y1 in row_runs:
                    if not 8<=y1-y0<=80:
                        continue
                    xs=np.flatnonzero((face[y0:y1]>127).any(0))
                    if len(xs):
                        proposals.append((int(xs[0]),y0,int(xs[-1])+1,y1,"projection",""))
            if "/staff" in entry["path"].lower() and rgba.shape[0]>=128:
                gradient=np.abs(np.diff(face.astype(float),axis=1)).mean(0)
                periods=[(max(float(gradient[k::period].mean()) for k in range(period)),period) for period in range(16,65) if rgba.shape[1]%period==0 and rgba.shape[0]%period==0]
                peak=max(v[0] for v in periods)
                period=min(v[1] for v in periods if v[0]>=peak*.9)
                for y in range(0,rgba.shape[0],period):
                    for x in range(0,rgba.shape[1],period):
                        proposals.append((x,y,x+period,y+period,"tile",""))
            for x0,y0,x1,y1,kind,detected in proposals:
                gray=face[y0:y1,x0:x1].copy()
                if kind=="tile":
                    n,labels,stats,_=cv2.connectedComponentsWithStats((gray>127).astype("uint8"))
                    for component in range(1,n):
                        x,y,w,h,area=stats[component]
                        if x<=1 or y<=1 or x+w>=gray.shape[1]-1 or y+h>=gray.shape[0]-1 or w>gray.shape[1]*.9 or h>gray.shape[0]*.9:
                            gray[labels==component]=0
                ys,xs=np.where(gray>127)
                if not len(xs):continue
                a,b,c,d=int(xs.min()),int(ys.min()),int(xs.max())+1,int(ys.max())+1
                gray=gray[b:d,a:c];x0+=a;y0+=b;x1=x0+gray.shape[1];y1=y0+gray.shape[0]
                if gray.shape[0]<7 or gray.shape[0]>100 or gray.shape[1]/gray.shape[0]<.2:continue
                sig=(mode,x0,y0,x1,y1)
                if sig in seen:continue
                seen.add(sig)
                r1,r2=recognize(gray,jp),recognize(gray,mix)
                if mode=="dark-ink" or (args.server_recognizer and kind!="tile"):
                    # Keep glyph-local recovery independent of sentence OCR:
                    # a failed or misspelled sentence must not discard its ink.
                    groups=column_groups(gray>127);spans=set(groups)
                    for first in range(len(groups)):
                        for last in range(first+1,min(first+4,len(groups))):
                            l,r=groups[first][0],groups[last][1]
                            if .50<=(r-l)/gray.shape[0]<=1.30:spans.add((l,r))
                    for l,r in sorted(spans):
                        glyph=gray[:,l:r];yy,xx=np.where(glyph>127)
                        if not len(xx):continue
                        left,top,right,bottom=int(xx.min()),int(yy.min()),int(xx.max())+1,int(yy.max())+1
                        glyph=glyph[top:bottom,left:right]
                        if not .45<=glyph.shape[1]/gray.shape[0]<=1.3:continue
                        aa,bb=recognize(glyph,jp),recognize(glyph,mix)
                        if bb[0].isascii() and len(bb[0])==1:
                            aa=recognize(glyph,en)
                        independent=aa[0]==bb[0] and len(aa[0])==1 and min(aa[1],bb[1])>=.95
                        context_options=[]
                        if not independent:
                            cc=recognize(glyph,en)
                            for label,score in (aa,bb,cc):
                                if len(label)==1 and score>=.95 and all(label in t and c>=.85 for t,c in (r1,r2)):
                                    context_options.append((score,label))
                            if not context_options:continue
                        ch=aa[0] if independent else max(context_options)[1]
                        if detected and ch not in unicodedata.normalize("NFKC",detected):continue
                        sample=dict(char=ch,source=sid,source_path=entry["path"],box=[x0+l+left,y0+top,x0+l+right,y0+bottom],line_height=gray.shape[0],y_offset=top,width=r-l,mode=mode,kind="independent-cut",line_text=detected,line_votes=0 if independent else 2,line_confidence=min(aa[1],bb[1]) if independent else max(context_options)[0],mask=glyph,single_readings=[aa,bb],independent_label=independent,context_readings=[r1,r2])
                        if ch in anchors:sample["anchor_similarity"]=similarity(glyph,anchors[ch])
                        own.append(sample)
                readings=[r1,r2]
                if any(t and t.isascii() for t,c in readings):readings.append(recognize(gray,en))
                votes=collections.Counter(t for t,c in readings if t and c>=.85)
                if not votes:continue
                text,count=votes.most_common(1)[0]
                confidence=max(c for t,c in readings if t==text)
                if kind=="tile":
                    choices=[(c,t) for t,c in readings if len(t)==1]
                    if not choices:continue
                    confidence,text=max(choices)
                    count=sum(t==text and c>=.85 for t,c in readings)
                info=dict(source=sid,path=entry["path"],box=[x0,y0,x1,y1],mode=mode,kind=kind,text=text,readings=readings,agreements=count,confidence=confidence)
                line_audit.append(info)
                if confidence<.92 or (count<2 and confidence<.975):continue
                bounds=segment(gray>127,text)
                if bounds is None:
                    info["status"]="no safe blank-column cuts";continue
                segments=list(zip(text,bounds,bounds[1:]))
                if mode=="dark-ink":
                    groups=column_groups(gray>127)
                    spans=set(groups)
                    for first in range(len(groups)):
                        for last in range(first+1,min(first+4,len(groups))):
                            l,r=groups[first][0],groups[last][1]
                            if .55<=(r-l)/gray.shape[0]<=1.30:spans.add((l,r))
                    for l,r in sorted(spans):
                        q=gray[:,l:r];yy,xx=np.where(q>127)
                        if not len(xx):continue
                        q=q[yy.min():yy.max()+1,xx.min():xx.max()+1]
                        aa,bb=recognize(q,jp),recognize(q,mix)
                        if aa[0]==bb[0] and len(aa[0])==1 and min(aa[1],bb[1])>=.95:
                            segments.append((aa[0],l,r))
                for ch,l,r in segments:
                    glyph=gray[:,l:r];yy,xx=np.where(glyph>127)
                    if not len(xx):continue
                    left,top,right,bottom=int(xx.min()),int(yy.min()),int(xx.max())+1,int(yy.max())+1
                    glyph=glyph[top:bottom,left:right]
                    if glyph.shape[1]>gray.shape[0]*1.4:continue
                    context_verified=sum(t==text and c>=.94 for t,c in readings)>=2 and kind!="tile" and .5<=(r-l)/gray.shape[0]<=1.6
                    sample=dict(char=ch,source=sid,source_path=entry["path"],box=[x0+l+left,y0+top,x0+l+right,y0+bottom],line_height=gray.shape[0],y_offset=top,width=r-l,mode=mode,kind=kind,line_text=text,line_votes=count,line_confidence=confidence,context_verified=context_verified,mask=glyph)
                    aa,bb=recognize(glyph,en if ch.isascii() else jp),recognize(glyph,mix)
                    sample["single_readings"]=[aa,bb]
                    sample["independent_label"]=aa[0]==bb[0] and len(aa[0])==1 and min(aa[1],bb[1])>=.95
                    if sample["independent_label"] and aa[0]!=ch:
                        sample["original_line_char"]=ch
                        ch=aa[0];sample["char"]=ch
                    if ch in anchors:sample["anchor_similarity"]=similarity(glyph,anchors[ch])
                    own.append(sample)
        # Unique characters, not repeated OCR variants, provide style evidence.
        bychar=collections.defaultdict(list)
        for s in own:
            if "anchor_similarity" in s and (s["independent_label"] or s["line_votes"]>=2):bychar[s["char"]].append(s["anchor_similarity"])
        scores={ch:max(values) for ch,values in bychar.items()}
        median=float(np.median(list(scores.values()))) if scores else 0
        accepted=bool(entry["style_accepted"]) or (len(scores)>=3 and median>=.80)
        style_audit.append(dict(source=sid,path=entry["path"],previously_accepted=entry["style_accepted"],accepted=accepted,distinct_anchors=len(scores),median=median,scores=scores,samples=len(own)))
        for sample in own:
            sample["style_accepted"]=accepted
            ident=f"s{len(all_samples):05d}";sample["id"]=ident
            sample["mask_file"]=f"samples/{ident}-mask.png";sample["rgba_file"]=f"samples/{ident}.png"
            Image.fromarray(sample["mask"]).save(args.output/sample["mask_file"])
            x0,y0,x1,y1=sample["box"]
            Image.fromarray(rgba[y0:y1,x0:x1]).save(args.output/sample["rgba_file"])
            all_samples.append(sample)
        print(f"source {si+1}/{len(inventory)} {entry['path']} samples={len(own)} style={accepted} score={median:.3f}",flush=True)
        write(args.output/"style-checks.json",style_audit)
        write(args.output/"lines.json",line_audit)
    # Near-identical glyphs in the same label class can confirm another source
    # family, e.g. the second credits atlas or repeated help-panel lettering.
    for iteration in range(3):
        refs=collections.defaultdict(list)
        for s in all_samples:
            if s["style_accepted"] and s["independent_label"]:refs[s["char"]].append(s)
        changed=False
        for report in style_audit:
            if report["accepted"]:continue
            own=[s for s in all_samples if s["source"]==report["source"] and s["independent_label"]]
            matches=collections.defaultdict(list)
            for s in own:
                if s["char"] in refs:matches[s["char"]].append(max(similarity(s["mask"],r["mask"]) for r in refs[s["char"]]))
            values={ch:max(scores) for ch,scores in matches.items()}
            median=float(np.median(list(values.values()))) if values else 0
            report["peer_check"]=dict(scores=values,median=median)
            if len(values)>=3 and median>=.88:
                report["accepted"]=True;changed=True
                for s in all_samples:
                    if s["source"]==report["source"]:s["style_accepted"]=True
        if not changed:break
    write(args.output/"style-checks.json",style_audit)
    bychar=collections.defaultdict(list)
    for s in all_samples:
        if s["style_accepted"]:bychar[s["char"]].append(s)
    chosen={};rejected=[]
    for ch,samples in sorted(bychar.items()):
        # Check best row-supported samples first; preserve every other sample.
        samples.sort(key=lambda s:(s["line_votes"],s["line_height"],s["line_confidence"]),reverse=True)
        verified=[]
        for s in samples:
            r1=recognize(s["mask"],en if ch.isascii() else jp)
            r2=recognize(s["mask"],mix)
            s["single_readings"]=[r1,r2]
            if not ch.isalnum() and not (r1[0]==r2[0]==ch and min(r1[1],r2[1])>=.98 and s["line_votes"]>=2):continue
            single=sum(t==ch and c>=.92 for t,c in (r1,r2))
            repeated=[o for o in samples if (o["source"],o["box"])!=(s["source"],s["box"]) and similarity(s["mask"],o["mask"])>=.88]
            if (single>=2) or (single>=1 and s["line_votes"]>=2) or (s["line_votes"]>=2 and repeated) or (ch.isalnum() and s.get("context_verified",False)):
                s["verification"]=dict(single_models=single,repeat_samples=len(repeated),context_verified=s.get("context_verified",False))
                verified.append(s)
        if verified:
            chosen[ch]=max(verified,key=lambda s:(s["line_height"],s["verification"]["single_models"],s["line_confidence"]))
        else:rejected.append(ch)
    font=TTFont(args.base_font);old=dict(font.getBestCmap());order=list(font.getGlyphOrder());added={}
    for ch,s in chosen.items():
        if ord(ch) in old:continue
        name=f"uni{ord(ch):04X}";pen=TTGlyphPen(None);scale=760/s["line_height"]
        mask=cv2.resize((s["mask"]>127).astype("uint8")*255,None,fx=4,fy=4,interpolation=cv2.INTER_NEAREST)
        contours,_=cv2.findContours(mask,cv2.RETR_LIST,cv2.CHAIN_APPROX_SIMPLE)
        for contour in contours:
            pts=contour[:,0,:]
            if len(pts)<3:continue
            coords=[(round(float(x)/4*scale+35),round(760-(float(y)/4+s["y_offset"])*scale)) for x,y in pts]
            pen.moveTo(coords[0])
            for pt in coords[1:]:pen.lineTo(pt)
            pen.closePath()
        font["glyf"][name]=pen.glyph();font["hmtx"][name]=(max(100,round(s["width"]*scale)+70),0);order.append(name)
        for table in font["cmap"].tables:
            if table.isUnicode():table.cmap[ord(ch)]=name
        added[ch]=s["id"]
        Image.fromarray(s["mask"]).save(args.output/"glyphs"/f"U{ord(ch):04X}.png")
    font.setGlyphOrder(order)
    version=args.output.name.split("-")[-1]
    for nid,value in {3:f"KinokoRasterExperimental-{version}",4:f"Kinoko Raster Experimental {version}",5:f"Version 0.{version}",6:f"KinokoRasterExperimental-{version}"}.items():
        font["name"].setName(value,nid,3,1,0x409);font["name"].setName(value,nid,1,0,0)
    target=args.output/"kinoko-raster-experimental.ttf";font.save(target)
    reopened=TTFont(target);assert set(old).issubset(reopened.getBestCmap())
    chars=sorted(c for c in reopened.getBestCmap() if c!=32)
    image=Image.new("RGB",(1000,math.ceil(len(chars)/20)*70+10),"white");draw=ImageDraw.Draw(image);f=ImageFont.truetype(str(target),32)
    for i,cp in enumerate(chars):
        x,y=i%20*50,i//20*70;draw.text((x+3,y),chr(cp),font=f,fill="black");draw.text((x+2,y+45),f"{cp:04X}",fill="black")
    image.save(args.output/"exported-font-preview.png")
    write(args.output/"samples.json",[{k:v for k,v in s.items() if k!="mask"} for s in all_samples])
    write(args.output/"added-glyphs.json",added)
    write(args.output/"rejected-characters.json",rejected)
    stats=dict(characters=len(chars),added_characters=len(added),sources=len(inventory),accepted_sources=sum(s["accepted"] for s in style_audit),samples=len(all_samples),rejected_characters=len(rejected),complete=False,manual_labels=False,manual_cuts=False,ocr_modifies_exported_pixels=False)
    write(args.output/"validation.json",stats);print(json.dumps(stats),flush=True)


if __name__=="__main__":main()
