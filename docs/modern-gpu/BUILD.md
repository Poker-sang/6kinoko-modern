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
