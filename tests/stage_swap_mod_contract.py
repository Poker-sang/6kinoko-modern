from pathlib import Path
import sys, json, hashlib
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).parents[1] / "tools"))
import create_stage_swap_mod as creator
import mod_session
import mod_manager

root = Path(sys.argv[1]); root.mkdir(parents=True, exist_ok=False)
payload = b"ACT1\x01\0\0\0\0\0\0\0fixture"
with patch.object(creator, "read_resource", return_value=payload) as read:
    project = creator.create(root / "reference", root / "project")
    read.assert_called_once_with(root / "reference", "data/map/w1-c02a.act")
assert (project / "data/map/w1-c01a.act").read_bytes() == payload
info, files = mod_session.read_mod(project)
assert "entrypoint" not in info and len(files) == 1
provenance = json.loads((project / "LOCAL-SOURCE.json").read_text())
assert provenance["sha256"] == hashlib.sha256(payload).hexdigest()
assert not provenance["transitions_modified"]
mod_manager.pack(project, root / "swap.kmod")
game = root / "game" / "kinoko_modern_gpu.exe"
game.parent.mkdir(); game.write_bytes(b"fixture, never executed")
installed, info = mod_manager.install(root / "swap.kmod", game)
mod_manager.enable(game, info["id"], info["version"], info["content_sha256"])
assert mod_manager.launch(game, prepare_only=True) == 0
try: creator.create(root / "reference", project)
except ValueError: pass
else: raise AssertionError("Overwrite accepted")
with patch.object(creator, "read_resource", return_value=b"invalid"):
    try: creator.create(root / "reference", root / "invalid")
    except ValueError: pass
    else: raise AssertionError("Invalid ACT accepted")
assert not (root / "invalid").exists()
print("PASS: resource-only swap, provenance, package/install/session preparation, invalid input and overwrite rejection")
