"""Audit a fresh MSVC modern-only build without executing any built code."""
import argparse
import hashlib
import json
import re
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build", type=Path, required=True)
parser.add_argument("--executable", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
cache = (args.build / "CMakeCache.txt").read_text(encoding="utf-8-sig")
for required in ("KINOKO_BUILD_LEGACY_COMPARISON:BOOL=OFF", "SDL_RENDER_D3D:BOOL=OFF"):
    if required not in cache.splitlines():
        raise SystemExit("Expected modern-only configuration: " + required)
compile_logs = sorted(args.build.rglob("CL.read.1.tlog"))
link_logs = sorted(args.build.rglob("link.read.1.tlog"))
if not compile_logs or not link_logs:
    raise SystemExit("Missing compiler/linker dependency records")
for target in ("kinoko_modern_gpu.dir", "SDL3-static.dir"):
    if not any(target in log.parts for log in compile_logs):
        raise SystemExit("Missing compilation record: " + target)
legacy_file = re.compile(r"(?:^|[\\/])(d3d9(?:types|caps)?|d3dx9[^\\/]*)\.(?:h|lib|dll)$", re.I)
violations = []
for log in compile_logs + link_logs:
    for line in log.read_text(encoding="utf-16").splitlines():
        if legacy_file.search(line.strip()):
            violations.append({"log": str(log), "input": line})
binary = args.executable.read_bytes()
markers = ("d3d9.dll", "d3dx9_33.dll", "Direct3DCreate9", "Direct3DCreate9Ex")
for marker in markers:
    if any(marker.encode(encoding).lower() in binary.lower() for encoding in ("ascii", "utf-16-le")):
        violations.append({"executable_marker": marker})
record = {
    "source_commit": (args.build / "source-commit.txt").read_text().strip(),
    "executable": str(args.executable),
    "sha256": hashlib.sha256(binary).hexdigest(),
    "compile_dependency_logs": len(compile_logs),
    "link_dependency_logs": len(link_logs),
    "checked_executable_markers": markers,
    "violations": violations,
    "passed": not violations,
    "executed_built_code": False,
    "scope": "Fresh modern-only MSVC dependency records and static executable markers; not runtime validation",
}
args.output.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
print(json.dumps(record, indent=2))
raise SystemExit(1 if violations else 0)
