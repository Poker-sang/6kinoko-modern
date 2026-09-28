"""Summarize actual full-game compiler errors; never execute built code."""
import argparse
import collections
import json
import re
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build',type=Path,required=True)
args=parser.parse_args()
manifest=json.loads((args.build/'artifacts.json').read_text(encoding='utf-8-sig'))
path=args.build/'build.log'
raw=path.read_bytes() if path.exists() else (args.build/'configure.log').read_bytes()
# Windows PowerShell's redirected native output is UTF-16LE with BOM.
log=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig',errors='replace')
errors=[]
seen=set()
for line in log.splitlines():
    match=re.search(r'^(.*?)\((\d+)(?:,\d+)?\):\s*(?:fatal )?error\s+([A-Z]+\d+):\s*(.*)',line)
    if match:
        file,number,code,message=match.groups()
        number=int(number)
    else:
        # Link failures have no source line; do not report a failed link as zero errors.
        match=re.search(r'^(.*?)\s*:\s*(?:fatal )?error\s+(LNK\d+):\s*(.*)',line)
        if not match: continue
        file,code,message=match.groups()
        number=None
    message=re.sub(r'\s+\[[^\]]+\]$','',message)
    key=(file,number,code,message)
    if key in seen: continue
    seen.add(key)
    errors.append(dict(file=file,line=number,code=code,message=message))
result=dict(source_commit=manifest['source_commit'],target=manifest['target'],
    full_build_succeeded=manifest['full_build_succeeded'],source_guards_enabled=True,
    executed_built_code=False,unique_diagnostics=len(errors),
    by_code=dict(collections.Counter(e['code'] for e in errors)),
    by_file=dict(collections.Counter(e['file'] for e in errors)),diagnostics=errors,
    limitations='Compiler/linker diagnostics from this build, not a complete pointer-flow or runtime correctness audit. Guard failures cause cascading diagnostics; counts are not independent migration tasks.')
(args.build/'blockers.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('diagnostics','by_file')},ensure_ascii=False))
