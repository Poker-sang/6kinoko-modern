# Explicit Mod startup and content registry

Source `fcdc52dc0556aba1af8ec471d4508b9d0b91be5c`, master. Implements explicit
manifest entrypoints, dependency-ordered startup after original boot, and the
Squirrel `KinokoMods` v1 registry for stage/enemy/boss/transformation factories.
This is the registration/dispatch layer, not new playable levels or enemies.
Map spawns, stage selection and numeric original actor-ID adapters remain future
integration work. API usage and restrictions are in [mods.md](mods.md).

## Delivery

`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-04/`

- `6kinoko-modern-windows-x64-fcdc52dc/Launch-Mods.cmd`: local package with both
  registration-probe and cyan-title-cursor installed/enabled.
- `registration-probe.kmod`: API self-check with no gameplay changes. Startup
  registers a factory and verifies its return value; this does not create an actor.
- `cyan-title-cursor.kmod`: previous visual sample.
- `6kinoko-modern-windows-x64-fcdc52dc.zip`: distributable package without DAT.

The raw and extracted game directories contain the three original DAT files,
verified by size and SHA256. No original archive was modified. Mod saves remain
isolated, and source/entrypoint changes select a different save identity.

## Validation and limits

Committed source before each fresh build/test batch. The initial VM regression
found unsupported newer closure syntax; the registry now uses explicit Squirrel
2.2 captures. Retained failed/intermediate logs are under mod-content-01/02.
Final `build-runs/mod-content-03` Windows x64 Release build succeeded. Native
file_archive/mod_resources/mod_scripts contracts passed (3/3). All three Python
session/manager/author suites passed. Checks cover real Squirrel compilation and
factory invocation, all four kinds, duplicate/missing IDs, stack restoration,
compile/runtime errors, entrypoint identity, absent resources and override rejection.
Startup errors block the game loop and return nonzero; partial script effects are
not rolled back, and scripts are not sandboxed.

Packaged tools built/installed both examples and prepared their combined session
without running the game. All 42 manifest hashes matched. No gameplay was run by
the agent. CI run 36595196824 was still in progress during preparation; no claim
of completed cross-platform validation for this source is made here.

## TAS integration

Changes are isolated to the Mod checkout and pushed to master. The other thread
was notified of runtime_host boot and application pre-loop hooks, plus source
script path handling in script_file.cpp. Existing native Info/identity APIs remain;
entrypoint access is additive. Resource-only catalogs stay KINOKOMODS1; scripted
catalogs use KINOKOMODS2. Mods with recording/playback remain explicitly blocked.
