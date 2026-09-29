import importlib.util,json,sys,tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('mods',Path(__file__).parents[1]/'tools/mod_session.py')
mods=importlib.util.module_from_spec(spec);spec.loader.exec_module(mods)
root=Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=False)
(root/'game.exe').write_bytes(b'fixture-not-executable');(root/'marisaA.dat').write_bytes(b'ordinary save')
def make(name,data,requires=None):
 d=root/name;(d/'data').mkdir(parents=True)
 (d/'data/probe.bin').write_bytes(data)
 (d/'mod.json').write_text(json.dumps({'format':1,'id':name,'version':'1.0','files':['data/probe.bin'],'requires':requires or []}))
 return d
def rejects(call):
 try:call()
 except (ValueError,OSError):return
 raise AssertionError('Expected rejection')
a=make('first',b'first');b=make('second',b'second',[{'id':'first','version':'1.0'}])
rejects(lambda:mods.prepare(root/'game.exe',[b]))
rejects(lambda:mods.prepare(root/'game.exe',[a,a]))
rejects(lambda:mods.prepare(root/'game.exe',[a,b]))
snapshot,report=mods.prepare(root/'game.exe',[a,b],True)
assert (snapshot/'assets/data/probe.bin').read_bytes()==b'second'
assert report['conflicts'][0]['winner']=='second'
assert not (Path(report['save_directory'])/'marisaA.dat').exists()
assert (root/'marisaA.dat').read_bytes()==b'ordinary save'
_,repeat=mods.prepare(root/'game.exe',[a,b],True);assert report['identity']==repeat['identity']
(b/'data/probe.bin').write_bytes(b'changed')
_,changed=mods.prepare(root/'game.exe',[a,b],True)
assert changed['identity']!=report['identity'] and (snapshot/'assets/data/probe.bin').read_bytes()==b'second'
for bad in ['../secret','data/../secret','data//bad','data/a:stream','data/a.','data/a/','data/\\bad']:
 rejects(lambda:mods.resource_path(bad))
assert mods.resource_path('data/PROBE.bin')=='data/probe.bin'
# The native contract validates SHA against independent standard vectors.
assert mods.digest(b'abc')=='ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad'
print('PASS: ordered dependencies, conflicts, snapshot isolation, stable/changing identity, save isolation and path rejection')
entry_mod=root/'scripted';entry_mod.mkdir()
script=entry_mod/'data/custom/scripted/main.nut';script.parent.mkdir(parents=True)
script.write_text('return 1;',encoding='utf-8')
manifest={'format':1,'id':'scripted','version':'1','files':['data/custom/scripted/main.nut']}
(entry_mod/'mod.json').write_text(json.dumps(manifest),encoding='utf-8')
before=mods.resolve([entry_mod])[-1]
manifest['entrypoint']='data/custom/scripted/main.nut'
(entry_mod/'mod.json').write_text(json.dumps(manifest),encoding='utf-8')
resolved=mods.resolve([entry_mod])
assert resolved[-1]!=before and resolved[-2].startswith('KINOKOMODS2\n')
assert 'E\tscripted\tdata/custom/scripted/main.nut\n' in resolved[-2]
manifest['entrypoint']='data/custom/scripted/missing.nut'
(entry_mod/'mod.json').write_text(json.dumps(manifest),encoding='utf-8')
rejects(lambda:mods.resolve([entry_mod]))
print('PASS: explicit entrypoint validation and identity')
manifest['entrypoint']='data/custom/scripted/main.nut'
(entry_mod/'mod.json').write_text(json.dumps(manifest),encoding='utf-8')
other=root/'entry-override';other.mkdir()
override=other/'data/custom/scripted/main.nut';override.parent.mkdir(parents=True)
override.write_text('return 2;',encoding='utf-8')
(other/'mod.json').write_text(json.dumps({'format':1,'id':'other','version':'1','files':['data/custom/scripted/main.nut']}),encoding='utf-8')
rejects(lambda:mods.resolve([entry_mod,other],True))
print('PASS: entrypoint override rejected even with general overrides enabled')
