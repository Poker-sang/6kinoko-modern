# SDL GPU foundation build

- Source commit: `088a52e96fa5aeae43a970e1a0d68c0c6a3fc2ab`.
- Unique build tree: `build-runs/modern-gpu-01`.
- Runtime: `runtime-builds/modern-gpu-01`.
- Win32 Release all-target compilation succeeded, including the GPU renderer,
  preview, vertex contract and both FXC SM5.1 shaders.
- Game EXE remains D3D9; three DAT files copied beside it and verified.
- Preview: `kinoko_gpu_preview.exe`; keep the adjacent `shaders` directory.
- `gpu-artifacts.json` records preview/contract/shader hashes;
  `modern-gpu-01-artifacts.json` records the game/DAT hashes.
- `gpu-shader-toolchain.txt` records pinned SDK, compiler path/hash and flags.
- Build/configure/DAT logs are retained in the build tree.
- No game, preview or tests executed. GPU validation is not established by build success.
- Prior modern-render-03 CI run 36318722902 succeeded; this is not the current GPU CI.

## Full GPU game: modern-gpu-game-02

- Source: `665a5dd7c037ab3772ca4db796885e245753a57d`.
- All-target Win32 Release compilation succeeded, including the full GPU game,
  legacy comparison game, both shaders, and GPU resource/matrix contracts.
- Primary handoff: `runtime-builds/modern-gpu-game-02/kinoko_modern_gpu.exe`.
- Keep adjacent `shaders/` and all three DAT. DAT staging was explicitly repeated
  and verified for the GPU EXE (the shared directory also contains the old EXE).
- `game-02-artifacts.json`: exact EXE/shader/DAT sizes and SHA256.
- `game-02-imports.txt`: compiler dependency inspection, no d3d9/d3dx9 DLL imports.
  The game uses SDL GPU; the Windows GPU implementation uses D3D12.
- `game-02-shader-toolchain.txt`: offline shader compiler identity and flags.
- `build-runs/modern-gpu-game-02/`: retained configuration/build/DAT/import logs.
- First complete intermediate build modern-gpu-game-01 also retained; do not
  substitute it for the hardened final version.
- No executable or test suite was run. Native-device contracts and new GPU/math
  contracts compiled only. Matrix rounding, depth fallback and offscreen depth
  sharing limitations are recorded in GAME.md.
- Foundation CI run 36320732448 succeeded; current full-game CI tracked separately.

## Explicit graphics interfaces: modern-gpu-api-04

- Source: `65eb6d72beebbfddb9e0df6dbe1c3e9ef08f9945`.
- Complete Win32 Release all-target build succeeded, including both games,
  original C contracts, GPU resources and the compile-only SDK vocabulary audit.
- Handoff: `runtime-builds/modern-gpu-api-04/kinoko_modern_gpu.exe`.
- All three DAT explicitly staged and size/SHA256 verified for the GPU EXE.
- Keep `shaders/` and the three DAT beside the EXE.
- `api-04-artifacts.json` records exact EXE/shader/DAT hashes and compiler input
  audits: the five modern game targets consume no D3D9 SDK headers.
- `api-04-imports.txt`: no d3d9/d3dx9 DLL imports. SDL GPU still uses D3D12.
- `api-04-shader-toolchain.txt`: shader compiler provenance.
- Vendored SDL itself retains its unused D3D9 driver/adapter SDK dependency;
  see GAME.md. Windows APIs and original x86 object layouts also remain.
- Modern-only mode separately succeeded at `a1cca83c28234bf4ad80a29508cb36ca3f7d704e`,
  in `modern-gpu-api-only-02`; its DAT/hash manifest is `api-only-02-artifacts.json`.
  This proves the comparison build can be omitted, not full-game portability.
- Retained intermediate batches: api-01 (implicit COM include failure), api-02
  (C fixture header boundary failure), api-only-01 (build/DAT succeeded but
  manifest generation assumed a tools directory), api-03 (remaining indirect C
  fixture header path). All logs and partial artifacts remain in build-runs and
  runtime-builds; no failed directory was overwritten.
- No game, preview, contract executable or local test suite was executed.
- User feedback for modern-gpu-game-02: currently normal, not agent validation.
- Previous CI 36322559102 succeeded. New workflow covers comparison ON and OFF;
  current source CI status is reported separately after push.

## D3D9-free default dependency graph: modern-no-d3d9-01

- Compiled source: `16360e4ca8998b9d6eaf67d780dcc0268f02cc1a`.
- Default Win32 Release build succeeded; it now builds/stages the modern game.
- Handoff: `runtime-builds/modern-no-d3d9-01/kinoko_modern_gpu.exe`.
- Three DAT copied beside this EXE and size/SHA256 verified. Keep the adjacent
  `shaders/` folder. EXE/DAT/shader hashes: `no-d3d9-01-artifacts.json`.
- The static audit scanned all 25 compiler and 4 linker dependency logs, including
  vendored SDL. No D3D9/D3DX SDK headers or import libraries were consumed.
- No D3D9 DLL/load-entry markers in the EXE; dumpbin also reports no D3D9/D3DX
  DLL imports (`no-d3d9-01-imports.txt`). This is static evidence, not execution.
- SDL D3D9 renderer is OFF; its header probe and Windows adapter loader are
  disabled by the documented local patch. The unused CRT loader was removed.
- Audit tool revision: `3fba044`; CI runs it for comparison=OFF. Its later commit
  also makes historical ACT/text build helpers explicitly request comparison mode;
  neither change alters the compiled game source above.
- `modern-no-d3d9-compare-01`, at the same compiled source, passed a separate
  complete historical comparison build and DAT staging. It is explicitly selected
  with `-LegacyComparison` and retains D3D9 by design; it is not the handoff EXE.
- All build/configuration/staging/audit logs and artifacts retained. No game,
  preview, contract executable or local test suite was run.
- User reports previous `modern-gpu-api-04` currently normal; not agent validation.
- Previous CI 36326020393 completed successfully. Current push CI is separate.
- Remaining migration: Windows services and original x86 runtime layouts; SDL GPU
  continues to use D3D12 on Windows. Removing D3D9 does not remove modern DirectX.

## Old backend retirement: modern-cleanup-02

- Source: `f2513bacad8e83a98a8db0fb4f2ae445fa9d9bff`.
- Handoff: `runtime-builds/modern-cleanup-02/kinoko_modern_gpu.exe`.
- Complete Win32 Release build succeeded, including 65 active contract executables.
  They are compiled only; neither they nor the game/preview were executed.
- Three DAT copied next to the EXE and verified by size/SHA256. Keep `shaders/`.
- `cleanup-02-artifacts.json`: source, EXE/DAT/shader hashes and static audit.
- All 98 compiler and 75 linker dependency logs are free of D3D9/D3DX SDK inputs.
  No D3D9 DLL/load-entry markers or DLL imports in the game EXE.
- Removed: old game target, clone-based GPU targets, SDK alias branches,
  KINOKO_GPU_GAME selection, D3DX import library and comparison build switches.
- Retained compatible tests now use the sole modern game libraries. Application
  and map-cache contracts explicitly link the project graphics implementation.
- COM-vtable fixtures and the mixed stage harness are non-build evidence under
  `docs/legacy-render-contracts`. This preserves assertions, but their unported
  coverage is not replaced or claimed by the 65 active contracts.
- `modern-cleanup-01` retained: game compiled, two standalone contracts lacked
  explicit links to modern graphics. The fresh final build fixes those links.
- Original decompiled evidence, historical logs/artifacts and currently used ABI,
  fixed-layout and Win32 code remain. This is old-render-backend retirement,
  not a claim that the entire game is portable or free of reverse-engineered code.
- Previous user feedback: modern-no-d3d9-01 currently normal. Previous CI
  36327063995 succeeded. Current push CI is tracked separately.
