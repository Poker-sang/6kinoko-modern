"""Package a committed native build without proprietary DAT or running any code."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("--runtime", type=Path, required=True)
parser.add_argument("--platform", choices=["linux-x64", "macos-universal", "windows-x64", "windows-x86"], required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--tests-passed", action="store_true", help="Record successful focused compatibility contracts, not gameplay")
args = parser.parse_args()
repo = Path(__file__).resolve().parent.parent
revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
name = "6kinoko-modern-" + args.platform + "-" + revision[:8]
windows = args.platform.startswith("windows-")
suffix = ".exe" if windows else ""
archive_suffix = ".zip" if windows else ".tar.gz"
args.output.mkdir(parents=True, exist_ok=True)
root = args.output / name
if root.exists() or (args.output / (name + archive_suffix)).exists():
    raise SystemExit("Refusing to overwrite retained package")
root.mkdir()
required = ["kinoko_modern_gpu"+suffix, "kinoko_gpu_transfer_contract"+suffix, "fonts/NotoSansCJKjp-Regular.otf", "fonts/LICENSE"]
extension = "dxbc" if windows else "msl" if args.platform == "macos-universal" else "spv"
required += ["shaders/sprite." + stage + "." + extension for stage in ["vert", "frag"]]
for relative in required:
    source = args.runtime / relative
    if not source.is_file() or not source.stat().st_size:
        raise SystemExit("Missing required runtime file: " + str(source))
    destination = root / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
shutil.copytree(repo / "config", root / "config")
shutil.copy2(repo / "tools/mod_session.py", root / "mod_session.py")
shutil.copy2(repo / "tools/mod_manager.py", root / "mod_manager.py")
shutil.copy2(repo / "tools/mod_author.py", root / "mod_author.py")
shutil.copy2(repo / "tools/mod_reference.py", root / "mod_reference.py")
shutil.copy2(repo / "tools/act_map_edit.py", root / "act_map_edit.py")
shutil.copy2(repo / "docs/modern-platform/mods.md", root / "MODS.md")
if windows:
    (root / "Choose-Mod-Stage.cmd").write_text('@echo off\ncd /d "%~dp0"\npython "%~dp0mod_manager.py" launch --game "%~dp0kinoko_modern_gpu.exe" --choose-stage\npause\n', encoding="ascii", newline="\r\n")
    (root / "Launch-Mods.cmd").write_text('@echo off\ncd /d "%~dp0"\npython "%~dp0mod_manager.py" launch --game "%~dp0kinoko_modern_gpu.exe"\npause\n', encoding="ascii", newline="\r\n")
    (root / "Install-Mod.cmd").write_text('@echo off\ncd /d "%~dp0"\nif "%~1"=="" (echo Drag a .kmod file onto this script. & pause & exit /b 1)\npython "%~dp0mod_manager.py" install-enable --game "%~dp0kinoko_modern_gpu.exe" "%~1"\npause\n', encoding="ascii", newline="\r\n")

shutil.copy2(repo / "docs/modern-platform/input-actions.md", root / "INPUT-ACTIONS.md")
if windows:
    shutil.copy2(repo / "tools/replay_session.ps1", root / "replay_session.ps1")
    shutil.copy2(repo / "docs/modern-platform/replay.md", root / "REPLAY.md")
    for mode, filename in [("record", "Record-Replay.cmd"), ("play", "Play-Replay.cmd")]:
        (root / filename).write_text('@echo off\ncd /d "%~dp0"\npowershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0replay_session.ps1" -Mode '+mode+'\npause\n', encoding="ascii", newline="\r\n")
for executable in ["kinoko_modern_gpu", "kinoko_gpu_transfer_contract"]:
    os.chmod(root / (executable+suffix), 0o755)
for diagnostic in [False, True]:
    if windows:
        launcher = root / ("run-with-diagnostics.cmd" if diagnostic else "launch.cmd")
        content = '@echo off\nsetlocal\ncd /d "%~dp0"\n'
        content += 'set "KINOKO_TRACE='+('1' if diagnostic else '0')+'"\n'
        content += 'set "KINOKO_TRACE_VERBOSE=0"\nset "KINOKO_TRACE_FILTER="\n'
        content += 'kinoko_modern_gpu.exe %*\n'
    else:
        extension_launcher = ".command" if args.platform == "macos-universal" else ".sh"
        launcher = root / (("Diagnose" if diagnostic else "Launch")+extension_launcher if args.platform == "macos-universal" else ("diagnose.sh" if diagnostic else "launch.sh"))
        content = '#!/bin/sh\ncd -- "$(dirname -- "$0")" || exit 1\n'
        content += 'export KINOKO_TRACE='+('1' if diagnostic else '0')+'\n'
        content += 'export KINOKO_TRACE_VERBOSE=0\nunset KINOKO_TRACE_FILTER\nexec ./kinoko_modern_gpu "$@"\n'
    launcher.write_text(content, encoding="utf-8", newline="\r\n" if windows else "\n")
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
    "Normal launch explicitly disables traces. Windows: launch.cmd; Linux: launch.sh; macOS: Launch.command.\n"
    "Diagnostics: run-with-diagnostics.cmd / diagnose.sh / Diagnose.command.\n"
    "Logs: retdec_trace.log on Windows; kinoko-trace.log on Linux/macOS, beside the executable.\n"
    "Optional hardware check: ./kinoko_gpu_transfer_contract verifies texture upload/readback without game data. Not executed in CI.\n"
    "This build is compiled and packaged; automated contracts do not establish gameplay validation.\n",
    encoding="utf-8")
for source in (repo / "third_party").rglob("*"):
    if source.is_file() and source.name.lower() in ["license", "license.txt", "copying", "copying.txt", "copyright"]:
        target = root / "licenses" / source.relative_to(repo / "third_party")
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
manifest = {"source_commit": revision, "platform": args.platform, "game_run": False,
            "tests_run": args.tests_passed, "original_dat_included": False, "files": {}}
for path in sorted(root.rglob("*")):
    if path.is_file():
        manifest["files"][path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
(root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
archive = args.output / (name + archive_suffix)
if windows:
    with zipfile.ZipFile(archive,"w",zipfile.ZIP_DEFLATED) as output:
        for path in sorted(root.rglob("*")):
            if path.is_file(): output.write(path,Path(name)/path.relative_to(root))
else:
    def modes(info):
        # Preserve executable permissions even when packaging on Windows.
        info.mode = 0o755 if info.isdir() or Path(info.name).name in [
            "kinoko_modern_gpu", "kinoko_gpu_transfer_contract", "launch.sh",
            "diagnose.sh", "Launch.command", "Diagnose.command"] else 0o644
        return info
    with tarfile.open(archive, "w:gz") as output:
        output.add(root, arcname=name, filter=modes)
print(archive)
