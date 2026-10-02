# Mod platform APIs and tooling

This repository owns the generic resource overlay, script registry, authoring,
installation and isolated-session APIs. Concrete maps, scripts, artwork, recipes,
content generators and their delivery history live in the local independent
project `C:/WorkSpace/6kinoko-mod-content`, which has no remote repository.
Engine packages do not bundle content examples or content-specific launchers.

## Prepare, install and launch

Python 3.10+ is required for the tools; ordinary game startup does not require
Python. Keep the three original DAT archives beside the executable. They remain
unchanged. Resource overlays support both replacements and new resource paths.

```powershell
python mod_author.py new C:/Mods/my-mod --id my-mod --name "My Mod"
python mod_author.py add C:/Mods/my-mod C:/Assets/item.cv2 data/custom/my-mod/item.cv2
python mod_author.py check C:/Mods/my-mod
python mod_manager.py pack C:/Mods/my-mod C:/Mods/my-mod.kmod
python mod_manager.py install-enable --game ./kinoko_modern_gpu.exe C:/Mods/my-mod.kmod
python mod_manager.py launch --game ./kinoko_modern_gpu.exe --prepare-only
python mod_manager.py launch --game ./kinoko_modern_gpu.exe
```

Run these from an engine package; in a checkout prefix tool paths with `tools/`.
`Launch-Mods.cmd`, `Install-Mod.cmd` and `Choose-Mod-Stage.cmd` are generic Windows
entrypoints. The optional stage selector lists registrations supplied by enabled
content; it does not provide a built-in campaign or stage.

`mod_manager` also provides list, enable, disable and move commands. Revisions are
stored under installed-mods/id/version/content-sha256. Choose version/hash when
more than one revision matches; no arbitrary latest version is selected. Move
uses a zero-based low-to-high load priority. Dependencies are validated before
saving changes. New installs and sessions preserve old files.

## Manifest and resources

```json
{
  "format": 1,
  "id": "my-mod",
  "name": "My Mod",
  "version": "1.0.0",
  "requires": [],
  "files": ["data/custom/my-mod/item.cv2"]
}
```

Paths are relative to mod.json, ASCII data/... paths, slash-normalized and
case-insensitive for lookup. Duplicate case variants, traversal, absolute paths,
escaping links and resources above 64 MiB are rejected. Dependencies use exact
id/version and must occur earlier in enabled order. Conflicts are errors unless
explicitly allowed, in which case later resources win and the report records
both winners and overridden resources.

Only unified archive-resource reads are overlaid. Saves, native libraries,
fonts/shaders beside the executable and loose platform files are outside this
API. Missing paths fall back to original archives; matched but invalid resources
fail rather than silently falling back. New resources require content scripts or
maps to refer to them; supplying a file does not register it automatically.

## Immutable sessions and saves

`mod_session.py --game GAME --mod DIRECTORY` prepares one or more projects in
low-to-high order. `--prepare-only` validates and snapshots without launching.
Each session retains effective resource files, catalog.tsv, session.json and logs.
The catalog SHA256 identifies ordered Mod IDs, versions, content and effective
resources independently of machine paths. Startup and resource opens verify
hashes; edits require a new session. Catalog loading is transactional.

Mod progress lives under mod-saves/full-identity and is separate from original
progress. Initial control configuration is copied, not original saved progress.
Changing content/order selects another save identity. Replay/recording with enabled
Mods is currently rejected until Mod identity is integrated into replay sessions.
There is no hot reload or automatic cleanup. Hash validation is integrity checking,
not a sandbox for untrusted Squirrel scripts.

## Script content API

An optional entrypoint must be a listed plain Squirrel .nut file below
`data/custom/<mod-id>/`. It runs after boot initialization. Its local `mod` handle
registers owner-scoped content through `mod.Register(kind,name,definition)`.
Categories are stage, enemy, boss and transformation; definitions require a display
name and create function. IDs are `<mod-id>:<name>`; duplicates are rejected.

KinokoMods API version 1 exposes For, List, Get, Create, Spawn, BindMapActor and
SpawnMap. List/Get return copies. Create takes an argument array. Enemy/boss
spawning requires an init function; map bindings require an unused 16-bit map ID
and explicit environment. Registrations do not automatically alter the original
world map, HUD or campaign. Content must implement those changes explicitly.
Boot-time function replacements can be overwritten by original ACT script reloads;
content that relies on them must handle the relevant resource-loading lifecycle.

## Generic authoring and checks

mod_author provides new/add/check/entrypoint and import-png/export-png operations.
PNG conversion optionally requires Pillow 11 or 12 and preserves unpremultiplied
BGRA alpha in CV2. mod_reference reads locally owned original DAT resources.
act_map_edit exposes bounded ACT editing through MapDocument, preserving supported
map/layer properties, timelines and resource records. Unsupported layouts fail
rather than being rewritten heuristically.

Engine contracts use temporary synthetic resources to verify archive fallback,
mutation rejection, dependencies, identities, isolated saves, registry/spawn
forwarding, error propagation, PNG conversion and bounded ACT edits. Concrete
content integration tests belong to the independent content project. Neither
native compilation nor these checks establishes gameplay acceptance.
