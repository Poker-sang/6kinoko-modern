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
