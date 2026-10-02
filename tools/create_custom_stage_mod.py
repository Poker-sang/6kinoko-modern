"""Build a small authored course over the normal first-stage entrance."""
import argparse
import hashlib
import json
from pathlib import Path
from act_map_edit import MapDocument
from mod_reference import read_resource
import mod_session

SOURCE = "data/map/w1-c01a.act"

def generate(reference, recipe=None):
    if recipe is None:
        recipe = Path(__file__).parent / "mod-authoring/short-course/map.json"
        if not recipe.exists(): recipe = Path(__file__).parents[1] / "examples/mod-authoring/short-course/map.json"
    spec = json.loads(Path(recipe).read_text(encoding="utf-8"))
    if spec.get("format") != 1: raise ValueError("Unsupported recipe format")
    width = spec["width"]
    if type(width) is not int or not 640 <= width <= 16384 or width % 32: raise ValueError("Invalid map width")
    terrain = []; edge = 0; heights = []
    for start, end, top in spec["floor"]:
        if any(type(v) is not int or v % 32 for v in (start,end,top)) or start != edge or not start < end <= width or not 640 <= top <= 960:
            raise ValueError("Floor must continuously cover width on the 32-pixel grid")
        heights.extend([top] * ((end-start)//32))
        edge = end
    if edge != width: raise ValueError("Incomplete floor")
    # Original w1-c01a keeps the continuous base grass beneath raised earth.
    # Exposed sides use the original cap/wall chips, not interior soil artwork.
    base = max(heights)
    for column, top in enumerate(heights):
        x = column * 32
        left = heights[column-1] if column else 1088
        right = heights[column+1] if column+1 < len(heights) else 1088
        if left > top and right > top:
            raise ValueError("Raised earth needs at least two columns for original left/right caps")
        terrain.append([1025 if left > top else 1027 if right > top else 1026,x,top])
        for y in range(top+32,1088,32):
            if y == base and top < base:
                chip = 1026
            else:
                chip = 1028 if y < left else 1029 if y < right else 1034
            terrain.append([chip,x,y])
    events = spec["events"]; enemies = spec["enemies"]
    if sorted(c[0] for c in events) != [1,3]: raise ValueError("Exactly one original spawn and goal required")
    for chip,x,y in events+enemies:
        if any(type(v) is not int for v in (chip,x,y)) or not 0 <= x < width-64 or not 0 <= y < 1088:
            raise ValueError("Actor placement outside course")
    original = read_resource(reference,SOURCE); document = MapDocument(original)
    # Reuse only verified original chip definitions, scripts and resources.
    for name,cells in (("terrain",terrain),("event",events),("enemy",enemies)):
        available = {c[0] for c in document.layers[name]["keys"][0]["cells"]}
        if any(c[0] not in available for c in cells): raise ValueError("Chip is not present in original layer: "+name)
    layers = dict(terrain=terrain,enemy=enemies,event=events,hidden=[],bg1=[],front=[],rail1=[],rail2=[])
    edited = document.edit(layers,width=width)
    return edited, spec, original

def create(reference, output, recipe=None):
    output = Path(output)
    if output.exists(): raise ValueError("Choose a new output directory")
    edited, spec, original = generate(reference, recipe)
    output.mkdir(parents=True,exist_ok=False)
    target = output/SOURCE; target.parent.mkdir(parents=True); target.write_bytes(edited)
    manifest = dict(format=1,id="short-course",name="Short course: three steps",version="1.0.1",requires=[],files=[SOURCE])
    (output/"mod.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    (output/"map.json").write_text(json.dumps(spec,indent=2)+"\n",encoding="utf-8")
    provenance = dict(source=SOURCE,source_sha256=hashlib.sha256(original).hexdigest(),sha256=hashlib.sha256(edited).hexdigest(),terrain_modified=True,script_modified=False)
    (output/"LOCAL-SOURCE.json").write_text(json.dumps(provenance,indent=2)+"\n",encoding="utf-8")
    mod_session.read_mod(output)
    return output

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-dir",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--recipe",type=Path)
    args = parser.parse_args()
    try: print(create(args.reference_dir,args.output,args.recipe))
    except (ValueError,OSError,KeyError,TypeError) as error: parser.exit(1,str(error)+"\n")
