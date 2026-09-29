"""Create Mod projects and import resources; PNG conversion optionally uses Pillow."""
import argparse
import json
from pathlib import Path
import struct
import mod_session as mods
from mod_manager import write_json


def create(directory, mod_id, name, version='1.0.0'):
    if not mods.TOKEN.fullmatch(mod_id) or not mods.TOKEN.fullmatch(version):
        raise ValueError('Invalid id/version')
    directory = Path(directory).resolve()
    directory.mkdir(parents=True, exist_ok=False)
    write_json(directory/'mod.json', {'format': 1, 'id': mod_id, 'name': name,
                                    'version': version, 'requires': [], 'files': []})
    return directory


def pillow():
    try:
        from PIL import Image
    except ImportError as error:
        raise ValueError('PNG conversion requires Pillow: python -m pip install "Pillow>=11,<13"') from error
    return Image


def png_bytes(source):
    Image = pillow()
    with Image.open(source) as image:
        if image.format != 'PNG':
            raise ValueError('Expected a PNG image')
        width, height = image.size
        if width < 1 or height < 1 or width*height*4+17 > mods.MAX_FILE:
            raise ValueError('Image exceeds 64 MiB decoded resource limit')
        if getattr(image, 'n_frames', 1) != 1:
            raise ValueError('Animated PNG is not supported')
        # Match the native 32-bit CV2 format: depth, width, height, row pixels,
        # reserved, then uncompressed BGRA. Preserve alpha; do not premultiply.
        pixels = image.convert('RGBA').tobytes('raw', 'BGRA')
    return struct.pack('<BIIII', 32, width, height, width, 0)+pixels


def add(directory, source, resource, *, png=False):
    directory = Path(directory).resolve()
    _, assets = mods.read_mod(directory, allow_empty=True)
    resource = mods.resource_path(resource)
    if resource in assets:
        raise ValueError('Resource already listed; choose a new path or edit the existing project file')
    if len(assets) >= 10000:
        raise ValueError('Resource count limit reached')
    source = Path(source)
    if not source.is_file() or source.stat().st_size > mods.MAX_FILE:
        raise ValueError('Source missing or exceeds 64 MiB')
    if png and not resource.endswith('.cv2'):
        raise ValueError('PNG import requires a .cv2 destination')
    target = directory/resource
    if not target.resolve().is_relative_to(directory):
        raise ValueError('Resource escapes project')
    if target.exists() or target.is_symlink():
        raise ValueError('Destination already exists; no files overwritten')
    data = png_bytes(source) if png else source.read_bytes()
    if len(data) > mods.MAX_FILE:
        raise ValueError('Resource exceeds 64 MiB')
    manifest = json.loads((directory/'mod.json').read_text(encoding='utf-8-sig'))
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('xb') as output:
        output.write(data)
    manifest['files'].append(resource)
    # Old manifests are retained in history. If publication fails, the imported
    # file stays unlisted for recovery; the previous profile remains valid.
    write_json(directory/'mod.json', manifest)
    return mods.read_mod(directory)[0]


def export_png(source, destination):
    source, destination = Path(source), Path(destination)
    if source.stat().st_size > mods.MAX_FILE:
        raise ValueError('Resource exceeds 64 MiB')
    data = source.read_bytes()
    if len(data) < 17:
        raise ValueError('Truncated CV2')
    depth, width, height, stride, reserved = struct.unpack('<BIIII', data[:17])
    if depth != 32 or reserved != 0 or not width or not height or stride < width or len(data) != 17+stride*height*4:
        raise ValueError('Only uncompressed 32-bit CV2 images are supported')
    image = pillow().frombytes('RGBA', (width, height), data[17:], 'raw', 'BGRA', stride*4)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open('xb') as output:
        image.save(output, format='PNG')


def set_entrypoint(directory, resource):
    directory = Path(directory).resolve()
    info, assets = mods.read_mod(directory)
    resource = mods.resource_path(resource)
    if resource not in assets or not resource.startswith('data/custom/'+info['id']+'/') or not resource.endswith('.nut'):
        raise ValueError('Entrypoint must be a listed data/custom/<mod-id>/*.nut resource')
    manifest = json.loads((directory/'mod.json').read_text(encoding='utf-8-sig'))
    manifest['entrypoint'] = resource
    write_json(directory/'mod.json', manifest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    cmd = commands.add_parser('new')
    cmd.add_argument('directory', type=Path); cmd.add_argument('--id', required=True)
    cmd.add_argument('--name', required=True); cmd.add_argument('--version', default='1.0.0')
    for name in ('add', 'import-png'):
        cmd = commands.add_parser(name)
        cmd.add_argument('directory', type=Path); cmd.add_argument('source', type=Path)
        cmd.add_argument('resource', help='New or replacement data/... resource path')
    cmd = commands.add_parser('check'); cmd.add_argument('directory', type=Path)
    cmd = commands.add_parser('entrypoint'); cmd.add_argument('directory', type=Path); cmd.add_argument('resource')
    cmd = commands.add_parser('export-png')
    cmd.add_argument('source', type=Path); cmd.add_argument('destination', type=Path)
    args = parser.parse_args()
    try:
        if args.command == 'new': print(create(args.directory, args.id, args.name, args.version))
        elif args.command in ('add', 'import-png'):
            print(json.dumps(add(args.directory, args.source, args.resource, png=args.command == 'import-png'), indent=2))
        elif args.command == 'check': print(json.dumps(mods.read_mod(args.directory)[0], indent=2))
        elif args.command == 'entrypoint': set_entrypoint(args.directory, args.resource)
        else: export_png(args.source, args.destination)
    except (ValueError, OSError, KeyError, TypeError) as error:
        parser.exit(1, 'Mod author error: '+str(error)+'\n')


if __name__ == '__main__':
    main()
