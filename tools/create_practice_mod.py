"""Build a local practice variant from the user's original first-stage map."""
import argparse
import hashlib
import json
from pathlib import Path
from mod_reference import read_resource
import mod_session

def create(reference, output, template=None):
    output = Path(output)
    if output.exists(): raise ValueError('Choose a new output directory')
    if template is None:
        template = Path(__file__).resolve().parent/'mod-authoring/practice-stage/main.nut'
        if not template.exists():
            template = Path(__file__).resolve().parents[1]/'examples/mod-authoring/practice-stage/main.nut'
    script = Path(template).read_bytes()
    source = 'data/map/w1-c01a.act'
    data = read_resource(reference, source)
    if len(data)<12 or data[:8]!=b'ACT1\x01\0\0\0': raise ValueError('Unexpected original ACT format')
    output.mkdir(parents=True, exist_ok=False)
    files = {'data/custom/practice-stage/main.nut':script, 'data/map/mods/practice-stage.act':data}
    for name, content in files.items():
        target = output/name; target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(content)
    manifest = {'format':1,'id':'practice-stage','name':'First-stage practice (original terrain)',
                'version':'1.0.0','requires':[],'entrypoint':'data/custom/practice-stage/main.nut','files':list(files)}
    (output/'mod.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (output/'LOCAL-SOURCE.json').write_text(json.dumps({'source':source,'sha256':hashlib.sha256(data).hexdigest(),
        'terrain_modified':False,'redistributed_by_repository':False},indent=2)+'\n',encoding='utf-8')
    mod_session.read_mod(output)
    return output

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-dir',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    try: print(create(args.reference_dir,args.output))
    except (ValueError,OSError) as error: parser.exit(1,str(error)+'\n')
