# Stage chooser and actor/map adapters

Package/tools source: `59e7585d`, master. Native binaries were built from
`f22ed1ed`; the only subsequent changes are Python stage-name validation and its
test. Native source is identical. Initial implementation is `6b112c5a`.

## Windows entry point

`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-05/6kinoko-modern-windows-x64-59e7585d/Choose-Mod-Stage.cmd`

This local package has registration-probe 1.1.0 installed and enabled. The SDL
startup chooser lists its Launcher demo. Selecting it invokes the stage factory,
which displays a message and returns to the original title. It is not a playable
level. Ordinary Launch-Mods.cmd does not open the chooser. The separate sample
package is `runtime-builds/mod-windows-05/registration-probe.kmod`.

Stage factories, Spawn, BindMapActor and SpawnMap usage and limits are documented
in [mods.md](mods.md#selecting-and-launching-a-mod-stage). These adapters reuse
original Actor initialization and InitXXXX map lookup, reject existing bindings,
and do not allocate map IDs or implement enemy/boss/transform behavior.

## Checks

Committed before builds/tests; retained build-runs/mod-stage-01 and mod-stage-02.
Windows x64 Release compiled successfully. Final native archive/resources/scripts
contracts passed (3/3). Three Python suites passed against initial implementation;
the manager suite passed again for the final Python-only name-validation change
under build-runs/mod-stage-tools-01.

Real Squirrel VM checks cover stage enumeration, factory dispatch, missing IDs,
stack restoration, actor argument forwarding, map environment/ID/layer forwarding,
and conflict/range rejection. Underlying CreateActor/SetInitFunctionByID/
CreateActorFromMap functions are mocked in the adapter test; this is not an
in-game enemy spawn observation. Launcher process execution is also mocked in
the environment-isolation check. The chooser UI/gameplay were not run by the
agent, and no user visual result is claimed.

Packaged sample creation, install and choose-stage prepare-only succeeded. All
43 distribution file hashes matched; three original DAT files were staged next
to both raw and packaged EXEs and size/SHA256 verified. Distribution ZIP excludes
DAT. All intermediate builds/logs retained. New CI is asynchronous; previous
fcdc52dc workflow 36595196824 completed successfully.

## Remaining work

A playable custom level still needs map/player/HUD assets and its scene/global
update lifecycle. This batch supplies explicit launch/dispatch and spawn adapters,
not an in-game world-map menu or level editor. Mod replay remains blocked until
identity integration. The separate TAS/rebuild checkouts were not modified.
