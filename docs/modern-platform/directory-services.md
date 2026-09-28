# SDL directory search and startup paths (2026-09-29)

User accepted modern-x64-locks-03. This is user feedback, not agent runtime testing.
User requested SDL/standard-library APIs in preference to hand-written OS backends.

ACT FindFirst/Next/Name/Close now use a small owner over SDL_GlobDirectory results.
No Win32/POSIX directory implementation or custom wildcard engine is maintained.
SDL owns OS enumeration, UTF-8 matching and temporary allocation. The owner keeps
native narrow filenames converted through std::filesystem for existing script/file
callers. ACT search IDs/counts/wrap and disposal ownership remain unchanged.

Matching is uniformly case-insensitive across platforms. SDL supports * and ?;
brackets are literal. A small pattern-normalization layer keeps *.* matching
extensionless names and foo.* matching foo. Only final-component wildcards are
accepted. Both separators work. Results include files and directories, exclude
`.`/`..`, and are a snapshot taken on first search; later filesystem mutations do
not update an existing search. The last filename remains valid after EOF. This is
not a complete emulation of DOS short-name/wildcard or live-enumeration behavior.

Startup uses SDL_GetBasePath and std::filesystem::current_path (SDL deliberately
has no API for changing cwd). For the standard .app layout, the shared path logic
maps SDL's Contents/Resources default to Contents/MacOS so DAT stay beside the
binary. Custom bundle layouts/Info.plist base-path overrides are not supported.
No native executable-path API is used. Startup failure aborts rather than reading
DAT from another cwd. Duplicate process-path cwd setup and the unused fixed-size
path cache are removed. Save/DAT formats are unchanged.

Contracts cover case-insensitive matching, literal brackets, missing paths,
extensionless names, directories, snapshot isolation, concurrent owners, EOF and
unbundled executable-directory selection. Compile only, no runtime tests executed.
Full Linux/macOS games still require window/COM/IME/font and other host migration.

The early directory-01/02 builds are retained as superseded implementation
attempts. The delivered revision uses SDL rather than their per-OS backends.

## Delivery evidence

Source `5816934cd1165d37646a6bfbc42e1aa45c8db72c`:
- `modern-x64-directory-03`: full x64 Release game compiled, plus directory,
  runtime-services and application contracts (contracts-build.log).
- `modern-directory-03`: Win32 game and 83 contracts compiled.
- Both staged/SHA256-verified the three original DAT; shaders staged.
- Both static D3D9 dependency audits passed. No game or contract ran.
- EXE: `runtime-builds/modern-x64-directory-03/kinoko_modern_gpu.exe`.
- Prior directory-01/02 builds and all logs remain; use directory-03 for SDL implementation.

Each source batch was committed before building. Deliverables and history are
backed up on codex/sdl-platform in the modern repository only.

CI [36446328292](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36446328292)
checked `97adcf363586da3778af2763bf2bc718e1379fff`: Linux, macOS and Windows portable
jobs passed, including directory/runtime-services contract compilation. The cloud
Win32 full-game job was still running at this snapshot; local Win32 full-game and
all-contract compilation passed as recorded above. CI does not run contracts.
