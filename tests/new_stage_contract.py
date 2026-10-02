"""Optional local-data resource integration, never starts the game."""
import json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/"tools"))
from act_map_edit import MapDocument
from mod_reference import read_resource
import create_new_stage_mod as creator
import mod_session,mod_manager
root=Path(sys.argv[1]);reference=Path(sys.argv[2]);root.mkdir(parents=True,exist_ok=False)
project=creator.create(reference,root/"project")
info,files=mod_session.read_mod(project)
assert set(files)=={creator.WORLD,creator.STAGE} and "entrypoint" not in info
original=MapDocument(read_resource(reference,creator.WORLD))
world=MapDocument((project/creator.WORLD).read_bytes())
assert original.edit({})==original.data
for name,l in original.layers.items():
 if name not in ["rail","symbol","event","point"]: assert world.layers[name]==l,name
 else:
  old=l["keys"][0]["cells"];new=world.layers[name]["keys"][0]["cells"]
  expected=[[202,x,y] if name=="rail" and (x,y)==(64,864) else [chip,x,y] for chip,x,y in old]
  assert all(c in new for c in expected)
  assert len(new)==len(old)+(2 if name=="rail" else 1)
assert [973,64,800] in world.layers["event"]["keys"][0]["cells"]
assert world.resources==original.resources
stage=MapDocument((project/creator.STAGE).read_bytes())
assert stage.properties["stName"]=="w1-c16a" and stage.properties["screenWidth"]==2560
# Verify directions against actual MCD, rather than trusting hardcoded chip labels.
b=read_resource(reference,"data/worldmap/worldmap.mcd");skip=struct.unpack_from("<I",b,8)[0];n,size=struct.unpack_from("<II",b,12+skip);p=20+skip;flags={}
for _ in range(n):
 row=b[p:p+size];p+=size+4;chip=struct.unpack_from("<I",row)[0];flags[chip]=struct.unpack_from("<I",row,16)[0]
assert flags[202]==flags[194]|16
assert flags[198]==48 and flags[206]==32
mod_manager.pack(project,root/"new-stage.kmod")
game=root/"game/kinoko_modern_gpu.exe";game.parent.mkdir();game.write_bytes(b"fixture, never executed")
_,info=mod_manager.install(root/"new-stage.kmod",game)
mod_manager.enable(game,info["id"],info["version"],info["content_sha256"])
assert mod_manager.launch(game,prepare_only=True)==0
try:creator.extend_world(world)
except ValueError:pass
else:raise AssertionError("Duplicate insertion accepted")
try:creator.create(reference,project)
except ValueError:pass
else:raise AssertionError("Overwrite accepted")
(root/"validation.json").write_text(json.dumps({"source_layers_preserved":True,"chip_flags_verified":True,"new_save_key":"w1-c16a","game_run":False},indent=2))
print("PASS: independent resource, original nodes/layers retained, actual road flags, immutable session and duplicate rejection")
