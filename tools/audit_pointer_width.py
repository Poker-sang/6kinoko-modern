"""Produce source-review candidates for x64 migration; not an exhaustive analyzer."""
import argparse
import collections
import json
import re
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
rules = {
    "pointer_to_int32": re.compile(r"\((?:u?int32_t|int)\)\s*\((?:u?intptr_t|uintptr_t)\)|static_cast<(?:std::)?u?int32_t>\(reinterpret_cast"),
    "legacy_address_boundary": re.compile(r"legacy::(?:pointer|address|field)|\b(?:pointer<|address\()"),
    "x86_abi": re.compile(r"__asm|__declspec\(naked\)|_M_IX86|sizeof\(void\s*\*\)\s*==\s*4|CMAKE_SIZEOF_VOID_P EQUAL 4"),
    "layout_assertion": re.compile(r"static_assert.*(?:sizeof|offsetof)"),
}
files = []
for directory in ("include/kinoko", "src/reconstructed", "src/platform", "src/squirrel", "cmake"):
    files.extend(p for p in (root / directory).rglob("*") if p.suffix in (".h", ".hpp", ".c", ".cpp", ".cmake"))
files.append(root / "CMakeLists.txt")
findings = []
for path in sorted(files):
    for number, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        if line.lstrip().startswith("//"):
            continue
        for category, pattern in rules.items():
            if pattern.search(line):
                findings.append({"file": path.relative_to(root).as_posix(), "line": number,
                                 "category": category, "source": line.strip()})
record = {
    "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
    "scope": "Maintained game/platform/binding headers and sources plus CMake; excludes original decompilation, third-party trees, tests and historical evidence",
    "limitations": "Line-based review candidates, not exhaustive pointer flow analysis. Layout assertions include valid file-format and scalar invariants, not only blockers. Unqualified address/pointer uses require manual triage.",
    "files_scanned": len(files),
    "counts": dict(collections.Counter(item["category"] for item in findings)),
    "findings": findings,
}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(record, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
print(json.dumps({key: value for key, value in record.items() if key != "findings"}, ensure_ascii=False))
