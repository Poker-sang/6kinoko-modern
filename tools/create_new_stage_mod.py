"""Add an independent normal-flow first-world stage using original unused slot 16."""
import argparse
import hashlib
import json
import struct
import colorsys
from pathlib import Path
from act_map_edit import MapDocument
from create_custom_stage_mod import generate
from mod_reference import read_resource
import mod_session

WORLD = "data/worldmap/worldmap.act"
STAGE = "data/map/w1-c16a.act"
STATUS = "data/system/stage/playerstatus.act"
LABEL = "data/custom/new-stage/stage16.cv2"
ENTRY = "data/custom/new-stage/main.nut"
BALLOON_MCD = "data/custom/new-stage/world-symbol.mcd"
BALLOON_IMAGE = "data/custom/new-stage/green-balloon.cv2"

def green_balloon(reference):
    data=read_resource(reference,'data/worldmap/worldmap.mcd')
    skip=struct.unpack_from('<I',data,8)[0];count,size=struct.unpack_from('<II',data,12+skip)
    p=20+skip+count*(size+4);textures=struct.unpack_from('<I',data,p)[0];p+=4
    for _ in range(textures):
        tid,n=struct.unpack_from('<II',data,p);p+=8;start=p;name=data[p:p+n].decode('cp932');p+=n
        if tid==18:
            image=read_resource(reference,name+'.cv2')
            bits,w,h,stride,reserved=struct.unpack('<BIIII',image[:17])
            if bits!=32 or len(image)!=17+stride*h*4:raise ValueError('Unexpected balloon texture format')
            pixels=bytearray(image[17:])
            for i in range(0,len(pixels),4):
                b,g,r,a=pixels[i:i+4]
                hue,sat,val=colorsys.rgb_to_hsv(r/255,g/255,b/255)
                if a and sat>.25 and (hue<.12 or hue>.93):
                    r,g,b=colorsys.hsv_to_rgb(.36,sat,val);pixels[i:i+3]=bytes((round(b*255),round(g*255),round(r*255)))
            new=BALLOON_IMAGE[:-4].encode('ascii')
            mcd=data[:start-4]+struct.pack('<I',len(new))+new+data[p:]
            return mcd,image[:17]+pixels
    raise ValueError('Original balloon texture missing')

def stage_label(reference):
    # Preserve original word and digit pixels, and the native CV2 BGRA wire format.
    one=read_resource(reference,"data/system/world/wmap_stage01.cv2")
    six=read_resource(reference,"data/system/world/wmap_stage06.cv2")
    if one[:17]!=six[:17] or struct.unpack('<BIIII',one[:17])!=(32,160,64,160,0):
        raise ValueError("Unexpected original stage-label format")
    pixels=bytearray(one[17:])
    for y in range(64): pixels[(y*160+108)*4:(y*160+160)*4]=bytes(52*4)
    digits=[]
    for data in (one,six):
        xs=[x for x in range(108,160) if any(data[17+(y*160+x)*4+3] for y in range(64))]
        digits.append((data,min(xs),max(xs)+1))
    total=sum(end-start for _,start,end in digits)+3
    cursor=108+(52-total)//2
    for data,start,end in digits:
        for y in range(64):
            pixels[(y*160+cursor)*4:(y*160+cursor+end-start)*4]=data[17+(y*160+start)*4:17+(y*160+end)*4]
        cursor+=end-start+3
    return one[:17]+pixels

def extend_world(document):
    def cells(name):
        keys = document.layers[name]["keys"]
        if len(keys) != 1 or "cells" not in keys[0]: raise ValueError("Expected one map key: " + name)
        return [list(c) for c in keys[0]["cells"]]
    additions = {"event": [[973,64,800]], "point": [[1014,64,800]],
                 "symbol": [[1023,32,736]], "rail": [[206,64,800],[198,64,832]]}
    updates = {}
    if [194,64,864] not in cells("rail") or [1006,64,864] not in cells("event"):
        raise ValueError("Original house/rail anchor does not match")
    # Slot 16 must be unused within the first world's 544x384 screen.
    if any(c[0] == 973 and 0 <= c[1] < 544 and 768 <= c[2] < 1152 for c in cells("event")):
        raise ValueError("First-world stage 16 already exists")
    for name, extra in additions.items():
        current = cells(name)
        occupied = {(c[1],c[2]) for c in current}
        if any((c[1],c[2]) in occupied for c in extra): raise ValueError("World placement collision: " + name)
        updates[name] = current + extra
    # Add up to the original right/down junction, preserving both old exits.
    updates["rail"] = [[202,x,y] if (x,y)==(64,864) else [chip,x,y] for chip,x,y in updates["rail"]]
    # Do not insert the branch under an original road mask/block.
    for name in document.layers:
        if name.startswith(("hidden_mask", "hidden_block")):
            if any((x,y) in {(64,800),(64,832)} for _,x,y in cells(name)):
                raise ValueError("New branch intersects original unlock layer")
    return document.edit(updates)

def create(reference, output, recipe=None):
    output = Path(output)
    if output.exists(): raise ValueError("Choose a new output directory")
    try: read_resource(reference, STAGE)
    except ValueError as error:
        if str(error) != "Original resource not found: " + STAGE: raise
    else: raise ValueError("New stage path already exists in original archives")
    if recipe is None:
        recipe=Path(__file__).parent/'mod-authoring/classic-1-1/map.json'
        if not recipe.exists(): recipe=Path(__file__).parents[1]/'examples/mod-authoring/classic-1-1/map.json'
    edited, spec, original = generate(reference, recipe)
    stage = MapDocument(edited).edit({}, name="w1-c16a")
    world_source = read_resource(reference, WORLD)
    world = extend_world(MapDocument(world_source))
    world_doc=MapDocument(world)
    chip=dict(world_doc.resources[0]['properties'],resourceID=2,stName='mod_mapchip',stChipFile=BALLOON_MCD)
    world=MapDocument(world_doc.add_chip_resource(chip)).add_map_layer('symbol','symbol_mod',2,[[1023,32,736]])
    balloon_mcd,balloon_image=green_balloon(reference)
    output.mkdir(parents=True, exist_ok=False)
    status_source=read_resource(reference,STATUS);status=MapDocument(status_source)
    template=next(r['properties'] for r in status.resources if r['properties']['stName']=='wmap_stage01')
    label_resource=dict(template,resourceID=max(r['properties']['resourceID'] for r in status.resources)+1,
        stName='mod_stage16',stTextureName=LABEL[:-4])
    status_data=status.add_texture(label_resource)
    entry=Path(__file__).parent/'mod-authoring/classic-1-1/main.nut'
    if not entry.exists():entry=Path(__file__).parents[1]/'examples/mod-authoring/classic-1-1/main.nut'
    for name, data in ((WORLD,world),(STAGE,stage),(STATUS,status_data),(LABEL,stage_label(reference)),(ENTRY,entry.read_bytes()),(BALLOON_MCD,balloon_mcd),(BALLOON_IMAGE,balloon_image)):
        target = output/name; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(data)
    manifest = dict(format=1,id="new-stage",name="Classic 1-1 inspired stage",version="1.1.0",requires=[],files=[WORLD,STAGE,STATUS,LABEL,ENTRY,BALLOON_MCD,BALLOON_IMAGE],entrypoint=ENTRY)
    (output/"mod.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    (output/"map.json").write_text(json.dumps(spec,indent=2)+"\n",encoding="utf-8")
    provenance = dict(world_source=WORLD,world_source_sha256=hashlib.sha256(world_source).hexdigest(),
        stage_template="data/map/w1-c01a.act",stage_template_sha256=hashlib.sha256(original).hexdigest(),
        files={WORLD:hashlib.sha256(world).hexdigest(),STAGE:hashlib.sha256(stage).hexdigest()},
        entrance=[64,800],save_key="w1-c16a",unlock="available from start",original_scripts_modified=False,
        presentation_entrypoint=ENTRY,course_description=spec.get('description','authored course'))
    (output/"LOCAL-SOURCE.json").write_text(json.dumps(provenance,indent=2)+"\n",encoding="utf-8")
    mod_session.read_mod(output)
    return output

if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-dir",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--recipe",type=Path)
    args=parser.parse_args()
    try: print(create(args.reference_dir,args.output,args.recipe))
    except (ValueError,OSError,KeyError,TypeError) as error: parser.exit(1,str(error)+"\n")
