# Additional stage and authored earth edges

Package/source snapshot: `d480f4ed`, Windows x64 Release, master.
Runtime compilation began at `9035004c`; subsequent commits changed only test
expectations and documentation, not compiled game sources or generators.
Remote master was fast-forwarded to `448babe4` before development and fetched
again before delivery. No changes were made to the rebuild or other thread's clone.

Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-10/6kinoko-modern-windows-x64-d480f4ed/Launch-Mods.cmd`

Only new-stage 1.0.1 is enabled. Follow the normal title/save/world flow, move up
from Marisa's house to the new 16 marker, then accept. Original stages are retained.
The new resource is data/map/w1-c16a.act, with its own original save key w1-c16a.
The world ACT adds the event, point, symbol and an up/down branch while retaining
the house's original right/down exits. No direct-start hooks or script changes.
Original world scripts perform the transition and return-position saving.

The user confirmed delivery 08 was playable but reported missing earth edges
and falling beside platforms. Both generators now select original left/right
caps 1025/1027 and exposed walls 1028/1029, retain interior soil 1034, and preserve
continuous base grass underneath raised earth as in the original first stage.
Caps share collision flags with middle grass, and walls with middle soil; no
physics change was introduced. In-game resolution of the reported falling
requires user verification. Single-column isolated raised earth is rejected
because this generator has no verified combined left/right cap.

Editable new-stage and short-course projects and .kmod packages are beside the
extracted game. short-course 1.0.1 is the corrected first-stage replacement and
is not enabled by default. All earlier deliveries remain intact.

## Validation

Windows build succeeded. ACT editor and real-resource integration checks passed,
including exposed/buried edges, base continuity, unchanged world layer bytes,
actual road flags, independent resource naming and immutable Mod preparation.
Native file_archive/mod_resources/mod_scripts checks passed (3/3).
The optional contract ran original worldmap/savedata/move bytecode and verified
node mapping, independent save/re-entry, delayed transition, return position and
road directions. Packaged manifest hashes matched and the packaged generator
reproduced both delivered ACT files. Three DAT files were copied beside both
raw and packaged EXEs and checked by size/SHA256. No game was run.

Logs/fixtures: build-runs/mod-terrain-01 and mod-terrain-03; compilation tree:
build-runs/mod-new-stage-08. Earlier failed fixture expectations and command
diagnostics are retained, alongside successful final logs. No artifacts deleted.

Stage SHA256: c5fb99450039917ddb45ca89125940d967a071a5184b4e96282d10b223d60b9e.
World SHA256: 777b096c31de32eed61edf5979ff8a70e55d33c88c2e8f2fdd5d427f31805edd.

## Scope

This uses unused original first-world slot 16 and a branch available from start.
It is not an arbitrary world/campaign API. Original completion totals remain
fixed, while clearing the extra stage contributes to original clearCount.
General campaign totals/unlock policies and world-overlay composition remain
future work; conflicting world ACT overlays are rejected by default.
