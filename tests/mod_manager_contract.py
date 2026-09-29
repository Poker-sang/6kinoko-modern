import sys,json,zipfile,stat,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
import mod_manager as manager
root=Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=False)
game=root/'game.exe';game.write_bytes(b'never execute fixture')
def create(name,files,requires=[]):
    d=root/name;d.mkdir()
    for path,data in files.items():
        p=d/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
    (d/'mod.json').write_text(json.dumps({'format':1,'id':name,'version':'1','requires':requires,'files':list(files)}))
    return d
def rejects(call):
    try:call()
    except (ValueError,OSError,zipfile.BadZipFile):return
    raise AssertionError('Expected rejection')
a=create('base',{'data/new-resource.bin':b'new'})
b=create('child',{'data/child.bin':b'child'},[{'id':'base','version':'1'}])
manager.pack(a,root/'a.kmod');manager.pack(a,root/'repeat.kmod')
assert (root/'a.kmod').read_bytes()==(root/'repeat.kmod').read_bytes()
rejects(lambda:manager.pack(a,root/'a.kmod'))
manager.pack(b,root/'b.kmod')
path,info=manager.install(root/'a.kmod',game);manager.install(root/'a.kmod',game)
manager.install(root/'b.kmod',game)
rejects(lambda:manager.enable(game,'child'))
manager.enable(game,'base');manager.enable(game,'child')
old=manager.profile_path(game).read_bytes()
rejects(lambda:manager.disable(game,'base'))
rejects(lambda:manager.move(game,'child',0))
assert manager.profile_path(game).read_bytes()==old
assert manager.launch(game,True)==0
manager.disable(game,'child');manager.disable(game,'base')
assert manager.launch(game,True)==0
assert (root/'history').exists()
assert (path/'data/new-resource.bin').read_bytes()==b'new'
# Manifest checksums and ZIP entry names are validated before accepting an install.
with zipfile.ZipFile(root/'a.kmod') as z:contents={n:z.read(n) for n in z.namelist()}
contents['data/new-resource.bin']=b'tampered'
with zipfile.ZipFile(root/'bad.kmod','w') as z:
    for n,data in contents.items():z.writestr(n,data)
rejects(lambda:manager.install(root/'bad.kmod',game))
for name in ('../escaped.bin','data/../escaped.bin','data\\escape','data/UPPER.bin'):
    with zipfile.ZipFile(root/'escape.kmod','w') as z:
        z.writestr('mod.json','{}');z.writestr('checksums.json','{}');z.writestr(name,b'x')
    rejects(lambda:manager.install(root/'escape.kmod',game))
assert not (root.parent/'escaped.bin').exists()
with zipfile.ZipFile(root/'link.kmod','w') as z:
    z.writestr('mod.json','{}');z.writestr('checksums.json','{}')
    item=zipfile.ZipInfo('data/link');item.external_attr=(stat.S_IFLNK|0o777)<<16;z.writestr(item,b'../../outside')
rejects(lambda:manager.install(root/'link.kmod',game))
(path/'data/new-resource.bin').write_bytes(b'changed')
rejects(lambda:manager.installed(game))
print('PASS: reproducible pack, no overwrite, install/hash validation, new resources, dependencies/order, transactional profile, disable-to-vanilla, traversal/link rejection')
from unittest.mock import patch
stage_game=root/'stage-game'/'game.exe';stage_game.parent.mkdir();stage_game.write_bytes(b'not executed')
rejects(lambda:manager.launch(stage_game,True,choose_stage=True))
scripted=create('scripted',{'data/custom/scripted/main.nut':b'return 1;'})
manifest=json.loads((scripted/'mod.json').read_text(encoding='utf-8'))
manifest['entrypoint']='data/custom/scripted/main.nut'
(scripted/'mod.json').write_text(json.dumps(manifest),encoding='utf-8')
manager.pack(scripted,root/'scripted.kmod');manager.install(root/'scripted.kmod',stage_game);manager.enable(stage_game,'scripted')
rejects(lambda:manager.launch(stage_game,True,stage='invalid'))
rejects(lambda:manager.launch(stage_game,True,stage='scripted:stage',choose_stage=True))
captured=[]
def fake_process(*args,**kwargs):captured.append(kwargs['env']);return 0
with patch.object(manager.subprocess,'call',fake_process),patch.dict(manager.os.environ,{'KINOKO_MOD_STAGE':'inherited:bad'}):
    manager.launch(stage_game,stage='scripted:stage')
    manager.launch(stage_game,choose_stage=True)
    manager.launch(stage_game)
    manager.launch(stage_game,stage='scripted:_stage')
assert captured[0]['KINOKO_MOD_STAGE']=='scripted:stage'
assert captured[1]['KINOKO_MOD_STAGE']=='@choose'
assert 'KINOKO_MOD_STAGE' not in captured[2]
assert captured[3]['KINOKO_MOD_STAGE']=='scripted:_stage'
print('PASS: stage selection validation and launch environment (process mocked)')
