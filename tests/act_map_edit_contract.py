from pathlib import Path
import struct,sys
sys.path.insert(0,str(Path(__file__).parents[1]/"tools"))
from act_map_edit import MapDocument

def u(v): return struct.pack("<I",v)
def text(s):
    b=s.encode("cp932"); return u(len(b))+b

def props(fields):
    out=b"\1"+u(len(fields))
    for name,(typ,value) in fields.items(): out+=text(name)+u(typ)
    for name,(typ,value) in sorted(fields.items()):
        out+=text(value) if typ==3 else bytes([value]) if typ==2 else struct.pack("<f" if typ==1 else "<i",value)
    return out
script=props({"compiled":(2,0),"filePath":(3,"")})+u(5)+b"test\0"
layout=props({"mapChipLeft":(0,0),"mapChipTop":(0,0),"mapChipRight":(0,32),"mapChipBottom":(0,32),"maxChipWidth":(0,32),"maxChipHeight":(0,32)})
layer=u(0x2618cf18)+props({"stName":(3,"terrain")})+u(1)+u(0xd933304d)+props({})+b"\1"+u(0xc9ca5c20)+layout+u(1)+u(12)+struct.pack("<Iii",7,0,0)+u(0)+script
original=b"ACT1\1\0\0\0"+u(0)+props({"screenWidth":(0,640),"stName":(3,"old")})+script+u(1)+layer+u(0)
d=MapDocument(original)
assert d.edit({})==original
out=d.edit({"terrain":[[7,64,32],[8,0,64]]},width=960,name="longer-name")
p=MapDocument(out)
assert p.properties["screenWidth"]==960 and p.properties["stName"]=="longer-name"
assert p.layers["terrain"]["keys"][0]["cells"]==[[8,0,64],[7,64,32]]
assert p.layers["terrain"]["keys"][0]["properties"]["mapChipBottom"]==96
assert out.count(script)==2
assert MapDocument(d.edit({"terrain":[]})).layers["terrain"]["keys"][0]["cells"]==[]
for bad in [original[:-1],original+b"extra",b"invalid",original[:12]+b"\0"+original[13:]]:
    try: MapDocument(bad)
    except ValueError: pass
    else: raise AssertionError("Invalid stream accepted")
for layers in [{"unknown":[]},{"terrain":[[7,-1,0]]},{"terrain":[[7,0]]}]:
    try: d.edit(layers)
    except ValueError: pass
    else: raise AssertionError("Invalid edit accepted")
print("PASS: lossless roundtrip, resized placements, ordering, bounds, script preservation, malformed input rejection")

# Mixed world-map ACT: preserve 2D key and timeline payload while editing terrain.
base=original[:-4]
extra_key=u(0xd933304d)+props({})+b"\1"+u(0x655cd5b0)+props({"alpha":(1,0.5)})
extra_layer=u(0x2618cf18)+props({"stName":(3,"sprite")})+u(1)+extra_key+u(1)+u(0x9902f2c0)+props({"beginTime":(0,0),"timeLength":(0,16)})+u(1)+u(0)+u(16)+script
# Locate the single document layer count after the document script.
count_offset=12+len(props({"screenWidth":(0,640),"stName":(3,"old")}))+len(script)
mixed=base[:count_offset]+u(2)+base[count_offset+4:]+extra_layer+u(1)+u(0xc6fdb98a)+props({"resourceID":(0,4),"stName":(3,"texture")})
m=MapDocument(mixed);assert m.edit({})==mixed
changed=m.edit({"terrain":[[7,32,64]]})
assert extra_layer in changed
assert MapDocument(changed).resources[0]["properties"]["stName"]=="texture"
try:m.edit({"sprite":[]})
except ValueError:pass
else:raise AssertionError("2D layer edit accepted")
print("PASS: mixed ACT2D/timeline/resource preservation and variable-length document name")
