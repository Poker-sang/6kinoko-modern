from pathlib import Path
import sys,json
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import mod_reference
import create_practice_mod as creator
import mod_session
import mod_manager

root=Path(sys.argv[1]);root.mkdir(parents=True,exist_ok=False)
mt=mod_reference.MT19937(5489)
assert [mt.next() for _ in range(10)]==[3499211612,581869302,3890346734,3586334585,545404204,4161255391,3922919429,949333985,2715962298,1323567403]
payload=b'ACT1\x01\0\0\0\0\0\0\0fixture'
with patch.object(creator,'read_resource',return_value=payload) as read:
    project=creator.create(root/'reference',root/'project')
    read.assert_called_once_with(root/'reference','data/map/w1-c01a.act')
assert (project/'data/map/mods/practice-stage.act').read_bytes()==payload
assert json.loads((project/'LOCAL-SOURCE.json').read_text())['terrain_modified'] is False
info,files=mod_session.read_mod(project)
assert info['id']=='practice-stage' and len(files)==2
mod_manager.pack(project,root/'practice.kmod')
try:creator.create(root/'reference',project)
except ValueError:pass
else:raise AssertionError('Overwrite accepted')
with patch.object(creator,'read_resource',return_value=b'invalid'):
    try:creator.create(root/'reference',root/'invalid')
    except ValueError:pass
    else:raise AssertionError('Invalid ACT accepted')
assert not (root/'invalid').exists()
print('PASS: reference MT vectors, generated project/package, provenance, no overwrite and invalid map rejection')
