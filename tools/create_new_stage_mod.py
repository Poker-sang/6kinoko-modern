"""Add an independent normal-flow first-world stage using original unused slot 16."""
import argparse
import hashlib
import json
from pathlib import Path
from act_map_edit import MapDocument
from create_custom_stage_mod import generate
from mod_reference import read_resource
import mod_session

WORLD = "data/worldmap/worldmap.act"
STAGE = "data/map/w1-c16a.act"

def extend_world(document):
    def cells(name):
        keys = document.layers[name]["keys"]
        if len(keys) != 1 or "cells" not in keys[0]: raise ValueError("Expected one map key: " + name)
        return [list(c) for c in keys[0]["cells"]]
    additions = {"event": [[973,64,800]], "point": [[1014,64,800]],
                 "symbol": [[973,64,800]], "rail": [[206,64,800],[198,64,832]]}
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
    edited, spec, original = generate(reference, recipe)
    stage = MapDocument(edited).edit({}, name="w1-c16a")
    world_source = read_resource(reference, WORLD)
    world = extend_world(MapDocument(world_source))
    output.mkdir(parents=True, exist_ok=False)
    for name, data in ((WORLD,world),(STAGE,stage)):
        target = output/name; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(data)
    manifest = dict(format=1,id="new-stage",name="New first-world stage 16",version="1.0.0",requires=[],files=[WORLD,STAGE])
    (output/"mod.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    (output/"map.json").write_text(json.dumps(spec,indent=2)+"\n",encoding="utf-8")
    provenance = dict(world_source=WORLD,world_source_sha256=hashlib.sha256(world_source).hexdigest(),
        stage_template="data/map/w1-c01a.act",stage_template_sha256=hashlib.sha256(original).hexdigest(),
        files={WORLD:hashlib.sha256(world).hexdigest(),STAGE:hashlib.sha256(stage).hexdigest()},
        entrance=[64,800],save_key="w1-c16a",unlock="available from start",scripts_modified=False)
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
