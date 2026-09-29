"""Package a committed native build without proprietary DAT or running any code."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile

parser = argparse.ArgumentParser()
parser.add_argument("--runtime", type=Path, required=True)
parser.add_argument("--platform", choices=["linux-x64", "macos-universal"], required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
repo = Path(__file__).resolve().parent.parent
revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
name = "6kinoko-modern-" + args.platform + "-" + revision[:8]
args.output.mkdir(parents=True, exist_ok=True)
root = args.output / name
if root.exists() or (args.output / (name + ".tar.gz")).exists():
    raise SystemExit("Refusing to overwrite retained package")
root.mkdir()
required = ["kinoko_modern_gpu", "kinoko_gpu_transfer_contract", "fonts/NotoSansCJKjp-Regular.otf", "fonts/LICENSE"]
extension = "msl" if args.platform == "macos-universal" else "spv"
required += ["shaders/sprite." + stage + "." + extension for stage in ["vert", "frag"]]
for relative in required:
    source = args.runtime / relative
    if not source.is_file() or not source.stat().st_size:
        raise SystemExit("Missing required runtime file: " + str(source))
    destination = root / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
os.chmod(root / "kinoko_modern_gpu", 0o755)
os.chmod(root / "kinoko_gpu_transfer_contract", 0o755)
launcher = root / ("Launch.command" if args.platform == "macos-universal" else "launch.sh")
launcher.write_text('#!/bin/sh\ncd -- "$(dirname -- "$0")" || exit 1\nexec ./kinoko_modern_gpu "$@"\n', encoding="utf-8")
os.chmod(launcher, 0o755)
(root / "README.txt").write_text(
    "6kinoko-modern / " + args.platform + "\nSource: " + revision + "\n\n"
    "Copy your original 6kinoko_a.dat, 6kinoko_b.dat and 6kinoko_c.dat into this folder, beside kinoko_modern_gpu.\n"
    "Original game data is not included. Keep shaders/ and fonts/ next to the executable.\n"
    "Launch with ./launch.sh on Linux, or Launch.command on macOS.\n"
    "Linux package targets Ubuntu 24.04 x86_64 or compatible newer systems with a Vulkan-capable graphics driver.\n"
    "macOS package contains Intel x86_64 and Apple Silicon arm64 code; Metal-capable macOS 14 or newer is targeted.\n"
    "The macOS build is unsigned/not notarized. Use the normal macOS Open confirmation for a trusted local build.\n"
    "The directory must be writable for saves. Run from an extracted folder, not inside the archive.\n"
    "KINOKO_TRACE=1 enables kinoko-trace.log beside the executable.\n"
    "Optional hardware check: ./kinoko_gpu_transfer_contract verifies texture upload/readback without game data. Not executed in CI.\n"
    "Compiled and packaged in CI; gameplay has not been validated on this platform.\n",
    encoding="utf-8")
for source in (repo / "third_party").rglob("*"):
    if source.is_file() and source.name.lower() in ["license", "license.txt", "copying", "copying.txt", "copyright"]:
        target = root / "licenses" / source.relative_to(repo / "third_party")
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
manifest = {"source_commit": revision, "platform": args.platform, "game_run": False,
            "tests_run": False, "original_dat_included": False, "files": {}}
for path in sorted(root.rglob("*")):
    if path.is_file():
        manifest["files"][path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
(root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
archive = args.output / (name + ".tar.gz")
with tarfile.open(archive, "w:gz") as output:
    output.add(root, arcname=name)
print(archive)
