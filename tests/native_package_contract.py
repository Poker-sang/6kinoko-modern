"""Package synthetic runtime files and check the engine/SDK distribution boundary."""
from pathlib import Path
import json
import subprocess
import sys
import zipfile

repo=Path(__file__).resolve().parents[1]
root=Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=False)
runtime=root/'runtime'
for name in ('kinoko_modern_gpu.exe','kinoko_gpu_transfer_contract.exe',
             'fonts/NotoSansCJKjp-Regular.otf','fonts/LICENSE',
             'shaders/sprite.vert.dxbc','shaders/sprite.frag.dxbc'):
    path=runtime/name;path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(b'synthetic packaging fixture, not executable')
subprocess.run([sys.executable,str(repo/'tools/package_native.py'),'--runtime',str(runtime),
                '--platform','windows-x64','--output',str(root/'package')],cwd=repo,check=True)
archive=next((root/'package').glob('*.zip'))
forbidden={'mod_author.py','mod_reference.py','mod_manager.py','mod_session.py','act_map_edit.py',
           'Choose-Mod-Stage.cmd','Launch-Mods.cmd','Install-Mod.cmd'}
with zipfile.ZipFile(archive) as bundle:
    assert not any(Path(name).name in forbidden for name in bundle.namelist())
    assert any(name.endswith('/MODS.md') for name in bundle.namelist())
    manifest=json.loads(bundle.read(next(name for name in bundle.namelist() if name.endswith('/manifest.json'))))
    assert not forbidden.intersection(manifest['files'])
assert all((repo/path).is_file() for path in ('src/platform/mod_resources.cpp','src/squirrel/mod_scripts.cpp',
                                           'tests/mod_resources_contract.cpp','tests/mod_scripts_contract.cpp'))
print('Game distribution excludes SDK; native Mod interfaces retained')
