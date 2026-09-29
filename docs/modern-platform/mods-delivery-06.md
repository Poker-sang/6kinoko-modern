# Locally generated first-stage practice candidate

Source/build: `391ee55186da196025bb52fd7b73865866754a0c`, master.
No native engine behavior or TAS interface changed this batch. Original DAT
loading and the existing Mod registration/selector remain the integration layer.

## Windows delivery

`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-06/6kinoko-modern-windows-x64-391ee551/Choose-Mod-Stage.cmd`

The extracted local package has practice-stage 1.0.0 installed and enabled.
Choose **First stage practice (original terrain, 99 lives)**. This is a gameplay
candidate that invokes the original stage lifecycle, not the previous message-box
demo. It has not been manually played by the agent or confirmed by the user.

The standalone generated `practice-stage.kmod` and editable `practice-stage/`
project are beside the game directory. The clean distribution ZIP contains only
the generator/template, not the generated proprietary map or original DAT.

## Actual scope

The local builder copies the user's original first-stage map to a new resource
path, leaving the original path/archive unchanged. Geometry and original enemies
are not redesigned. The new script supplies practice initialization (99 lives,
paused initial-map timer), ends original title/logo presentation, and delegates
to original InitStage. It restores temporary transition hooks before invoking
the original return-to-title path and also restores them after startup exceptions.
Original submap warps retain their behavior. See mods.md for remaining limitations.

Copied map: 19378 bytes, SHA256
`1e0606933f87e5ffd51911a36584801379daefbf62741f1f379bb672a8455ff4`.
Original terrain source is data/map/w1-c01a.act; LOCAL-SOURCE.json records provenance.
All generated maps/packages remain local, not committed to the repository.

## Validation

Committed before build/test. Windows x64 Release build succeeded. Native
file_archive/mod_resources/mod_scripts checks passed (3/3); four Python suites
session/manager/author/practice passed. The practice regression uses the actual
template in Squirrel 2.2.2 with mocked game APIs to check startup order, flags,
return-to-title and exception restoration. It does not exercise actual map loading,
physics, rendering or pause UI. Static original-script evidence is documented in
[mod-practice-evidence.md](mod-practice-evidence.md).

Packaged generator read the local original DAT, created the project, packed and
installed it, then prepared a stage-specific session without game execution.
All 46 package manifest files matched. Three DAT files were staged beside raw and
extracted EXEs and size/SHA256 checked. Logs/fixtures remain under
build-runs/mod-practice-01; research under mod-playable-research-01. No artifacts
were deleted. CI 36599806043 was queued at delivery preparation; no new CI success
is claimed. Earlier stage-chooser source 59e7585d completed CI successfully.
