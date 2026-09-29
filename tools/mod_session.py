"""Prepare and launch immutable, ordered resource overlays (Python 3.10+)."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import tempfile

TOKEN = re.compile(r"[a-z0-9][a-z0-9._-]{0,126}\Z")
MAX_FILE = 64 * 1024 * 1024

def digest(data):
    return hashlib.sha256(data).hexdigest()

def canonical(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=True, separators=(',', ':')).encode('ascii')

def resource_path(text):
    if not isinstance(text, str) or '\\' in text or not text.startswith('data/') or len(text)>512:
        raise ValueError('Resource path must be a forward-slash data/... path')
    if any(not p or p in ('.','..') or p.endswith((' ','.')) or not re.fullmatch(r'[a-zA-Z0-9._ -]+',p) for p in text.split('/')):
        raise ValueError('Invalid resource path: '+text)
    return text.lower()

def checked_file(root, relative):
    path=root.joinpath(*PurePosixPath(relative).parts)
    if not path.resolve().is_relative_to(root.resolve()) or path.is_symlink() or not path.is_file():
        raise ValueError('Missing/escaping resource: '+relative)
    if path.stat().st_size>MAX_FILE:
        raise ValueError('Resource exceeds 64 MiB: '+relative)
    return path

def read_mod(directory):
    directory=Path(directory).resolve()
    path=directory/'mod.json'
    if path.stat().st_size>1024*1024: raise ValueError('Manifest too large')
    manifest=json.loads(path.read_text(encoding='utf-8-sig'))
    if manifest.get('format')!=1 or set(manifest)-{'format','id','name','version','requires','files'}:
        raise ValueError('Unsupported manifest fields/version')
    for field in ('id','version'):
        if not isinstance(manifest.get(field),str) or not TOKEN.fullmatch(manifest[field]):
            raise ValueError('Invalid '+field)
    files=manifest.get('files')
    if not isinstance(files,list) or not files or len(files)>10000: raise ValueError('Expected a nonempty files list')
    assets={}
    for relative in files:
        key=resource_path(relative)
        if key in assets:raise ValueError('Case-colliding resource: '+key)
        source=checked_file(directory,relative)
        assets[key]=(source,digest(source.read_bytes()))
    requirements=manifest.get('requires',[])
    if not isinstance(requirements,list):raise ValueError('requires must be a list')
    for requirement in requirements:
        if not isinstance(requirement,dict) or set(requirement)!={'id','version'} or any(not isinstance(requirement[k],str) or not TOKEN.fullmatch(requirement[k]) for k in ('id','version')):
            raise ValueError('Dependencies require exact id/version')
    identity={'id':manifest['id'],'version':manifest['version'],'requires':requirements,'files':{key:value[1] for key,value in sorted(assets.items())}}
    return {**identity,'content_sha256':digest(canonical(identity))},assets

def prepare(game, mod_directories, allow_overrides=False):
    game=Path(game).resolve()
    if not game.is_file():raise ValueError('Game executable not found')
    if not mod_directories:raise ValueError('Select at least one Mod, or use ordinary launch without Mods')
    modules=[];effective={};owners={};conflicts=[];known={}
    for directory in mod_directories:
        info,assets=read_mod(directory)
        if info['id'] in known:raise ValueError('Duplicate Mod id: '+info['id'])
        for dependency in info['requires']:
            if known.get(dependency['id'])!=dependency['version']:raise ValueError('Dependency must be enabled earlier at exact version: '+dependency['id'])
        known[info['id']]=info['version'];modules.append(info)
        for key,value in assets.items():
            if key in effective:conflicts.append({'resource':key,'previous':owners[key],'winner':info['id']})
            effective[key]=value;owners[key]=info['id']
    if conflicts and not allow_overrides:raise ValueError('Resource conflict; use --allow-overrides for explicit later-wins order: '+json.dumps(conflicts))
    catalog='KINOKOMODS1\n'+''.join('M\t'+m['id']+'\t'+m['version']+'\t'+m['content_sha256']+'\n' for m in modules)
    catalog+=''.join('F\t'+key+'\t'+effective[key][1]+'\n' for key in sorted(effective))
    identity=digest(catalog.encode('ascii'))
    cache=game.parent/'mod-sessions';cache.mkdir(exist_ok=True)
    snapshot=Path(tempfile.mkdtemp(prefix=identity[:12]+'-',dir=cache))
    for key,(source,expected) in effective.items():
        target=snapshot/'assets'/key;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,target)
        if digest(target.read_bytes())!=expected:raise ValueError('Mod changed while preparing snapshot: '+key)
    (snapshot/'catalog.tsv').write_bytes(catalog.encode('ascii'))
    saves=game.parent/'mod-saves'/identity
    first=not saves.exists();saves.mkdir(parents=True,exist_ok=True)
    if first:
        for name in ('keyconfig.dat','input-actions.cfg'):
            source=game.parent/name
            if source.is_file():shutil.copyfile(source,saves/name)
    report={'format':1,'identity':identity,'mods':modules,'conflicts':conflicts,'save_directory':str(saves),'game':str(game)}
    (snapshot/'session.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    return snapshot,report

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game',type=Path,required=True)
    parser.add_argument('--mod',type=Path,action='append',required=True,help='repeat in low-to-high priority order')
    parser.add_argument('--allow-overrides',action='store_true')
    parser.add_argument('--prepare-only',action='store_true')
    args=parser.parse_args()
    try:
        snapshot,report=prepare(args.game,args.mod,args.allow_overrides)
        print('Mod session:',snapshot);print('Content identity:',report['identity']);print('Saves:',report['save_directory'])
        for conflict in report['conflicts']:print('Override:',conflict)
        if args.prepare_only:return 0
        env=os.environ.copy();env['KINOKO_MOD_CATALOG']=str(snapshot/'catalog.tsv');env.pop('KINOKO_REPLAY_MODE',None)
        with (snapshot/'process.log').open('w',encoding='utf-8') as log:
            result=subprocess.run([str(args.game.resolve()),'--save-dir',report['save_directory']],cwd=args.game.resolve().parent,env=env,stdout=log,stderr=log)
        (snapshot/'exit-code.txt').write_text(str(result.returncode),encoding='ascii')
        return result.returncode
    except (ValueError,OSError,KeyError,TypeError) as e:
        parser.exit(1,'Mod error: '+str(e)+'\n')
if __name__=='__main__':raise SystemExit(main())
