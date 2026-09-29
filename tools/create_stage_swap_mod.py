"""Build a resource-only first-stage replacement from locally owned original data."""
import argparse
import hashlib
import json
from pathlib import Path
from mod_reference import read_resource
import mod_session

SOURCE = "data/map/w1-c02a.act"
TARGET = "data/map/w1-c01a.act"

def create(reference, output):
    output = Path(output)
    if output.exists():
        raise ValueError("Choose a new output directory")
    data = read_resource(reference, SOURCE)
    if len(data) < 12 or data[:8] != b"ACT1\x01\0\0\0":
        raise ValueError("Unexpected original ACT format")
    output.mkdir(parents=True, exist_ok=False)
    target = output / TARGET
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    manifest = {"format": 1, "id": "first-stage-swap", "name": "First entrance, second-stage terrain",
                "version": "1.0.0", "requires": [], "files": [TARGET]}
    (output / "mod.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    provenance = {"source": SOURCE, "target": TARGET, "sha256": hashlib.sha256(data).hexdigest(),
                  "terrain_modified": False, "transitions_modified": False,
                  "redistributed_by_repository": False}
    (output / "LOCAL-SOURCE.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    mod_session.read_mod(output)
    return output

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        print(create(args.reference_dir, args.output))
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + "\n")
