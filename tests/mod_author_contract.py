import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).parents[1]/'tools'))
import mod_author as author
import mod_manager as manager
import mod_session as session
from PIL import Image

root = Path(sys.argv[1]).resolve()
root.mkdir(parents=True, exist_ok=False)

def rejects(call):
    try: call()
    except (ValueError, OSError): return
    raise AssertionError('Expected rejection')

project = author.create(root/'project', 'new-content', 'New content')
rejects(lambda: author.create(project, 'new-content', 'Collision'))
rejects(lambda: manager.pack(project, root/'empty.kmod'))
image = Image.new('RGBA', (3, 2))
pixels = [(255, 0, 0, 255), (0, 255, 0, 128), (0, 0, 255, 0),
          (1, 2, 3, 4), (5, 6, 7, 8), (255, 255, 255, 255)]
image.putdata(pixels); image.save(root/'image.png')
author.add(project, root/'image.png', 'data/custom/new-image.cv2', png=True)
cv2 = project/'data/custom/new-image.cv2'
assert cv2.read_bytes()[:17] == struct.pack('<BIIII', 32, 3, 2, 3, 0)
assert cv2.read_bytes()[17:25] == bytes((0, 0, 255, 255, 0, 255, 0, 128))
author.export_png(cv2, root/'roundtrip.png')
with Image.open(root/'roundtrip.png') as restored:
    assert restored.size == (3, 2) and list(restored.getdata()) == pixels
before = (project/'mod.json').read_bytes()
rejects(lambda: author.add(project, root/'image.png', 'data/custom/new-image.cv2', png=True))
rejects(lambda: author.add(project, root/'image.png', 'data/../../escape.cv2', png=True))
rejects(lambda: author.add(project, root/'image.png', 'data/wrong.bin', png=True))
assert (project/'mod.json').read_bytes() == before
rejects(lambda: author.export_png(cv2, root/'roundtrip.png'))
(root/'script.nut').write_text('return 42;', encoding='utf-8')
author.add(project, root/'script.nut', 'data/custom/new-script.nut')
assert list((project/'history').glob('*.json'))
manager.pack(project, root/'new-content.kmod')
game = root/'game.exe'; game.write_bytes(b'not executed')
installed, info = manager.install(root/'new-content.kmod', game)
assert info == session.read_mod(project)[0]
manager.enable(game, 'new-content')
assert manager.launch(game, prepare_only=True) == 0
assert (installed/'data/custom/new-script.nut').read_bytes() == b'return 42;'
print('Mod author contracts passed (including PNG/BGRA/alpha and package integration)')
