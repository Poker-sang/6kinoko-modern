# Mod runtime interface

The game contains the C++ resource overlay and Squirrel content registry. Mod
authoring, ACT editing, PNG/CV2 conversion, archive reading, package installation
and session preparation belong to the separate `6kinoko-mod-sdk` project.
The game distribution contains no SDK Python modules or SDK launch scripts.

The external SDK takes an explicit game executable path. It installs `.kmod`
packages beside that executable and prepares an immutable resource catalog,
then launches the game with `KINOKO_MOD_CATALOG` set to that catalog. The game
verifies the catalog and resource hashes and loads the overlay. Without a
catalog, ordinary game startup uses the original resources and saves.

Resource lookup is case-insensitive under `data/`. Missing overlay paths fall
back to the original DAT archives; invalid matched resources fail. Libraries,
fonts, shaders and saves are outside the resource overlay. Original DAT files
remain external to the distribution. Multiple Mods and dependencies are resolved
by the SDK before launch; conflicting resources require explicit override.

An optional plain Squirrel entrypoint below `data/custom/<mod-id>/` runs after
boot initialization. Its owner-scoped `mod.Register(kind,name,definition)` handle
registers stage, enemy, boss and transformation content. Definitions require a
display name and create function. `KinokoMods` API version 1 exposes For, List,
Get, Create, Spawn, BindMapActor and SpawnMap. Registrations do not automatically
modify the original campaign, world map or HUD. SDK-generated adapters are Mod
content, loaded through the same ordinary resource and script interfaces.

The optional `KINOKO_MOD_STAGE` startup selection accepts `<mod-id>:<content-id>`
or `@choose` for the game's registered-stage selector. Mod progress uses the
catalog identity in a separate save directory. Changing content/order selects a
different identity; replay with enabled Mods is currently rejected. Hash checks
provide integrity, not a sandbox for scripts. Hot reload is not supported.

Native resource and script registry contracts remain in this repository and its
CI. Python authoring, installation and session tests live in the SDK. SDK setup
and commands are documented in that project's README and docs/authoring.md.
