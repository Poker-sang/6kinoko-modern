"""Audit every original CV2 with text detection, retaining all positive regions."""
from pathlib import Path
import argparse, hashlib, json
import cv2
import numpy as np
from PIL import Image
from rapidocr import RapidOCR
from rapidocr.utils.typings import LangRec, OCRVersion, ModelType
from export_unlabeled import decode


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--entries",type=Path,default=Path("docs/font-resource-investigation/entries.json"))
    p.add_argument("--reference",type=Path,default=Path("C:/WorkSpace/6kinoko"))
    p.add_argument("--output",type=Path,required=True)
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    (args.output/"sources").mkdir()
    engine=RapidOCR(params={"Rec.lang_type":LangRec.JAPAN,"Rec.ocr_version":OCRVersion.PPOCRV4,"Rec.model_type":ModelType.MOBILE,"Global.use_cls":False,"Global.log_level":"error"})
    entries=[e for e in json.loads(args.entries.read_text(encoding="utf-8")) if e["path"].lower().endswith(".cv2")]
    records=[];cache={}
    for i,entry in enumerate(entries):
        record=dict(entry);records.append(record)
        try:rgba,depth=decode(entry,args.reference)
        except Exception as exc:
            record["error"]=str(exc);continue
        digest=hashlib.sha256(rgba.tobytes()).hexdigest()
        if digest in cache:
            record.update(cache[digest]);record["duplicate_pixels"]=True;continue
        rgb=(rgba[:,:,:3].astype(float)*rgba[:,:,3:]/255).astype("uint8")
        scale=min(3,1280/max(rgb.shape[:2]))
        temp=cv2.resize(rgb,None,fx=scale,fy=scale,interpolation=cv2.INTER_CUBIC)
        result=engine(cv2.cvtColor(temp,cv2.COLOR_RGB2BGR),use_cls=False)
        detections=[]
        if result.boxes is not None:
            for box,text,score in zip(result.boxes,result.txts,result.scores):
                detections.append(dict(box=(np.asarray(box)/scale).tolist(),text=text,confidence=float(score)))
        value=dict(pixel_digest=digest,depth=depth,width=rgba.shape[1],height=rgba.shape[0],detections=detections)
        cache[digest]=value;record.update(value)
        if detections:Image.fromarray(rgba).save(args.output/"sources"/f"{digest}.png")
        if i%50==0:
            (args.output/"inventory.json").write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding="utf-8")
            print(f"audited {i+1}/{len(entries)}; text-positive {sum(bool(r.get('detections')) for r in records)}",flush=True)
    (args.output/"inventory.json").write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding="utf-8")
    print(json.dumps(dict(entries=len(records),unique_bitmaps=len(cache),decode_errors=sum('error' in r for r in records),positive_resources=sum(bool(r.get('detections')) for r in records))),flush=True)


if __name__=="__main__":main()
