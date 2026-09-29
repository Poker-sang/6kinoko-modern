"""Pack, install, enable and launch resource Mods. No third-party Python packages."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
import uuid
import zipfile
import mod_session as mods

MAX_TOTAL=512*1024*1024

def write_json(path,value):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():
        history=path.parent/'history';history.mkdir(exist_ok=True)
        shutil.copyfile(path,history/(path.stem+'-'+uuid.uuid4().hex+'.json'))
    temporary=path.with_name(path.name+'.'+uuid.uuid4().hex+'.tmp')
    temporary.write_bytes(mods.canonical(value)+b'\n');os.replace(temporary,path)

def pack(directory,destination):
    directory=Path(directory).resolve();destination=Path(destination).resolve()
    info,assets=mods.read_mod(directory)
    # Normalize names in the archive so case-sensitive platforms use identical paths.
    original=json.loads((directory/'mod.json').read_text(encoding='utf-8-sig'))
    manifest={**original,'files':sorted(assets)}
    contents={'mod.json':mods.canonical(manifest)+b'\n'}
    for name,(source,expected) in assets.items():
        data=source.read_bytes()
        if mods.digest(data)!=expected:raise ValueError('Source changed during pack: '+name)
        contents[name]=data
    if sum(map(len,contents.values()))>MAX_TOTAL:raise ValueError('Package exceeds 512 MiB')
    contents['checksums.json']=mods.canonical({name:mods.digest(data) for name,data in sorted(contents.items())})+b'\n'
    destination.parent.mkdir(parents=True,exist_ok=True)
    if destination.exists():raise ValueError('Package already exists; choose a new filename')
    temporary=destination.with_name(destination.name+'.'+uuid.uuid4().hex+'.tmp')
    with zipfile.ZipFile(temporary,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,data in sorted(contents.items()):
            item=zipfile.ZipInfo(name,(1980,1,1,0,0,0));item.compress_type=zipfile.ZIP_DEFLATED;item.external_attr=(stat.S_IFREG|0o644)<<16
            archive.writestr(item,data)
    # Atomic no-overwrite publication on the same filesystem.
    os.link(temporary,destination);temporary.unlink()
    return info

def install(package,game):
    root=Path(game).resolve().parent/'installed-mods';root.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(package) as archive:
        items=archive.infolist()
        if len(items)>10002 or len(items)<3:raise ValueError('Invalid package entry count')
        seen=set();total=0
        for item in items:
            name=item.filename
            if name not in ('mod.json','checksums.json') and mods.resource_path(name)!=name:raise ValueError('Package paths must be normalized')
            if name in seen or item.is_dir() or stat.S_ISLNK(item.external_attr>>16) or item.flag_bits&1:raise ValueError('Duplicate/link/directory/encrypted entry')
            seen.add(name);total+=item.file_size
            limit=1024*1024 if name in ('mod.json','checksums.json') else mods.MAX_FILE
            if item.file_size>limit or total>MAX_TOTAL:raise ValueError('Package exceeds size limit')
        if not {'mod.json','checksums.json'}<=seen:raise ValueError('Missing package metadata')
        checks=json.loads(archive.read('checksums.json'))
        if not isinstance(checks,dict) or set(checks)!=seen-{'checksums.json'}:raise ValueError('Checksum inventory mismatch')
        manifest=json.loads(archive.read('mod.json'))
        if not isinstance(manifest,dict) or not isinstance(manifest.get('files'),list) or set(manifest['files'])!=seen-{'mod.json','checksums.json'}:raise ValueError('Manifest inventory mismatch')
        staging=Path(tempfile.mkdtemp(prefix='.install-',dir=root))
        # No extractall: only validated names are ever written; incomplete staging is retained.
        for name in sorted(checks):
            data=archive.read(name)
            if mods.digest(data)!=checks[name]:raise ValueError('Checksum mismatch: '+name)
            target=staging/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    info,_=mods.read_mod(staging)
    target=root/info['id']/info['version']/info['content_sha256']
    target.parent.mkdir(parents=True,exist_ok=True)
    if target.exists():
        current,_=mods.read_mod(target)
        if current!=info:raise ValueError('Existing installation changed')
        return target,info
    staging.rename(target)
    return target,info

def installed(game):
    root=Path(game).resolve().parent/'installed-mods'
    result=[]
    if root.exists():
        for manifest in sorted(root.glob('*/*/*/mod.json')):
            if manifest.relative_to(root).parts[0].startswith('.'):continue
            info,_=mods.read_mod(manifest.parent)
            if manifest.parent.name!=info['content_sha256'] or manifest.parent.parent.name!=info['version'] or manifest.parent.parent.parent.name!=info['id']:
                raise ValueError('Installed Mod content/path mismatch: '+str(manifest))
            result.append((manifest.parent,info))
    return result

def profile_path(game):return Path(game).resolve().parent/'mod-profile.json'
def profile(game):
    path=profile_path(game)
    if not path.exists():return {'format':1,'enabled':[],'allow_overrides':False}
    if path.stat().st_size>1024*1024:raise ValueError('Profile too large')
    value=json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(value,dict) or value.get('format')!=1 or not isinstance(value.get('enabled'),list) or type(value.get('allow_overrides')) is not bool:raise ValueError('Invalid profile')
    ids=set()
    for entry in value['enabled']:
        if not isinstance(entry,dict) or set(entry)!={'id','version','content_sha256'}:raise ValueError('Invalid profile entry')
        if entry['id'] in ids:raise ValueError('Duplicate enabled Mod')
        ids.add(entry['id'])
    return value

def directories(game,value):
    available={(info['id'],info['version'],info['content_sha256']):path for path,info in installed(game)}
    result=[]
    for entry in value['enabled']:
        key=(entry['id'],entry['version'],entry['content_sha256'])
        if key not in available:raise ValueError('Enabled Mod is missing or changed: '+entry['id'])
        result.append(available[key])
    return result

def save_profile(game,value):
    selected=directories(game,value)
    if selected:mods.resolve(selected,value['allow_overrides'])
    write_json(profile_path(game),value)

def enable(game,mod_id,version=None,content_hash=None,allow_overrides=False):
    candidates=[info for _,info in installed(game) if info['id']==mod_id and (version is None or info['version']==version) and (content_hash is None or info['content_sha256']==content_hash)]
    if len(candidates)!=1:raise ValueError('Select exactly one installed version/hash; use list')
    choice=candidates[0];value=profile(game)
    entry={k:choice[k] for k in ('id','version','content_sha256')}
    prior=next((i for i,e in enumerate(value['enabled']) if e['id']==mod_id),None)
    if prior is None:value['enabled'].append(entry)
    else:value['enabled'][prior]=entry
    value['allow_overrides']=allow_overrides or value['allow_overrides']
    save_profile(game,value)

def disable(game,mod_id):
    value=profile(game)
    if not any(e['id']==mod_id for e in value['enabled']):raise ValueError('Mod is not enabled')
    value['enabled']=[e for e in value['enabled'] if e['id']!=mod_id];save_profile(game,value)

def move(game,mod_id,index):
    value=profile(game);entries=value['enabled']
    if not 0<=index<len(entries):raise ValueError('Position is outside enabled list')
    found=next((e for e in entries if e['id']==mod_id),None)
    if found is None:raise ValueError('Mod is not enabled')
    entries.remove(found);entries.insert(index,found);save_profile(game,value)

def launch(game,prepare_only=False):
    game=Path(game).resolve()
    if not game.is_file():raise ValueError('Game not found')
    value=profile(game);selected=directories(game,value)
    env=os.environ.copy();env.pop('KINOKO_MOD_CATALOG',None);env.pop('KINOKO_REPLAY_MODE',None)
    if not selected:
        print('No Mods enabled; ordinary game and ordinary saves.')
        return 0 if prepare_only else subprocess.call([str(game)],cwd=game.parent,env=env)
    snapshot,report=mods.prepare(game,selected,value['allow_overrides'])
    print(json.dumps({'snapshot':str(snapshot),**report},ensure_ascii=False,indent=2))
    if prepare_only:return 0
    env['KINOKO_MOD_CATALOG']=str(snapshot/'catalog.tsv')
    with (snapshot/'process.log').open('w',encoding='utf-8') as log:
        result=subprocess.call([str(game),'--save-dir',report['save_directory']],cwd=game.parent,env=env,stdout=log,stderr=log)
    (snapshot/'exit-code.txt').write_text(str(result));return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    commands=parser.add_subparsers(dest='command',required=True)
    cmd=commands.add_parser('pack');cmd.add_argument('directory',type=Path);cmd.add_argument('output',type=Path)
    for name in ('install','install-enable','list','enable','disable','move','launch'):
        cmd=commands.add_parser(name);cmd.add_argument('--game',type=Path,required=True)
        if name in ('install','install-enable'):cmd.add_argument('package',type=Path)
        if name in ('enable','disable','move'):cmd.add_argument('id')
        if name=='enable':cmd.add_argument('--version');cmd.add_argument('--hash');cmd.add_argument('--allow-overrides',action='store_true')
        if name=='move':cmd.add_argument('position',type=int,help='zero-based enabled order')
        if name=='launch':cmd.add_argument('--prepare-only',action='store_true')
    args=parser.parse_args()
    try:
        if args.command=='pack':print(json.dumps(pack(args.directory,args.output),indent=2))
        if args.command in ('install','install-enable'):
            path,info=install(args.package,args.game);print('Installed:',path)
            if args.command=='install-enable':enable(args.game,info['id'],info['version'],info['content_sha256'])
        if args.command=='list':print(json.dumps({'installed':[info for _,info in installed(args.game)],'profile':profile(args.game)},ensure_ascii=False,indent=2))
        if args.command=='enable':enable(args.game,args.id,args.version,args.hash,args.allow_overrides)
        if args.command=='disable':disable(args.game,args.id)
        if args.command=='move':move(args.game,args.id,args.position)
        if args.command=='launch':return launch(args.game,args.prepare_only)
        return 0
    except (ValueError,OSError,KeyError,TypeError,zipfile.BadZipFile) as error:parser.exit(1,'Mod error: '+str(error)+'\n')
if __name__=='__main__':raise SystemExit(main())
