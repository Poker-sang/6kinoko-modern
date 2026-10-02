# Preserve HUD receiver binding when entering the Mod stage

Source/package snapshot: `3c07d276`, new-stage 1.1.1, Windows x64.
Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-13/6kinoko-modern-windows-x64-3c07d276/Launch-Mods.cmd`

The user reported accepting the Mod entrance stopped input without entering the
stage, while rendering/music continued. The delivery 12 session process.log
records repeated `the index 'stage' does not exist` at main.nut line 7, called
from original Update_WorldScroll. This is a script receiver error, not a renderer
hang. Original PlayerStatus.OnCreate publishes SetWorld with bindenv(this) on
the public ACT object; stage and resources live on its distinct global table.
The presentation wrapper omitted that binding. It now binds explicitly to
PlayerStatus.global, preserving the original API's receiver contract.

The regression fixture now models the public ACT facade and internal global as
separate tables. After committing, the old script reproduced the user's exact
stage-slot error, and the corrected script passed with actual original
world/savedata/move/playerstatus/effect bytecode. Original node/house/blank label
restoration, green animation and visibility synchronization also passed. The
previous fixture used one table for both roles and missed this bug.

The optional native contract was compiled in a fresh build tree
build-runs/mod-status-bind-02; results and old/fixed logs are retained under
build-runs/mod-status-bind-01. Real-resource integration passed; packaged file
hashes matched, packaged tools reproduced all seven resources, and required
DAT files were copied/checksummed beside the packaged EXE. Preparation did not
start the game. Actual entry/playability still requires user verification.

The game executable is the unchanged previous Windows x64 Release binary from
runtime-builds/mod-new-stage-08. Only the Mod script/version and regression
fixture changed. Course, green balloon and Stage 16 artwork remain as delivery
12. All old packages, user logs, saves and test artifacts remain untouched.
