# Resource Mod delivery — 2026-09-29

Isolated checkout: `C:\WorkSpace\6kinoko-mods` (name requested without modern).
Branch: `codex/mod-overlays`, based on `65b2eb01` in the existing game repository.
Game/source build: `188fcb9e7f90a17dc792d570efb4a4b0c0f4f31c`.
No changes made to the other thread's working directory or TAS editor.

## Delivered behavior

Ordered manifest-based resource overlays, exact dependency requirements,
explicit conflict resolution, per-file SHA256, immutable prepared snapshots,
Mod-set-specific saves, runtime identity API and an original cyan title cursor.
Ordinary DAT remains untouched and the resource reader falls back on misses.
Disabling Mods restores DAT reads. This is the resource foundation, not a Mod UI,
level editor or an implementation of new gameplay objects.

Windows package:
`C:\WorkSpace\6kinoko-mods\runtime-builds\mod-windows-01\6kinoko-modern-windows-x64-188fcb9e`

Use `Launch-Cursor-Mod.cmd` to enable the sample; `launch.cmd` for ordinary play.
Python 3.10+ is required only by the Mod launcher. See `MODS.md` in the package.
The inherited game binary/package naming has not been renamed in this branch.
The original three DAT files were staged beside raw and packaged EXEs, size/hash
verified. The standard ZIP excludes proprietary DAT. All package manifest hashes
were verified after packaging. No actual game was launched by the agent.

## Checks

- Windows x64 Release game, GPU transfer executable and resource checks compiled.
- `file_archive_contract` passed: real unified reader returns Mod bytes ahead of
  synthetic DAT, and returns the DAT bytes after Mod disable.
- `mod_resources_contract` passed: independent SHA256 vectors (including million-a),
  catalog load, case/slash lookup, mutation rejection and clearing invalid sets.
- Python launcher checks passed: exact ordered dependencies, duplicate IDs,
  conflict errors/explicit winner, stable/changing hashes, retained immutable
  snapshots, path rejection and ordinary-save isolation.
- Packaged example launcher `--prepare-only` succeeded without executing game.
- Build/logs: `build-runs/mod-overlays-02/`; fixtures and earlier builds retained.

CI run: https://github.com/Poker-sang/6kinoko-modern/actions/runs/36589721122
All seven jobs passed: Windows x86/x64 full builds and focused contracts,
Linux/macOS full compilation, and three portable compile jobs. No Linux/macOS
gameplay or native contracts were executed for this batch.

## Coordination

TAS thread owns its independent `codex/tas-bridge` changes and confirmed that it
will not merge this batch during its current implementation. Integration surface:
`application_runtime.cpp` immediately before `kinoko_replay_start()`; no option
parser/replay/input/render implementation was edited. Public `mods::active()` and
`mods::identity()` provide the ordered module metadata and canonical SHA256.
Mod+record/play is explicitly rejected until session identity integration is done.
No automatic merge into master or the TAS branch is performed.
