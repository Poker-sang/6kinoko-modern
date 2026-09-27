# 6kinoko-modern

SDL3 platform migration fork of 6kinoko-rebuild. This first step replaces the window, physical input and audio output and upgrades Ogg/Vorbis. The full game is still Windows x86; portable modules build separately. No gameplay validation is claimed.

## Build / 构建

```sh
cmake -S . -B build-runs/platform-01 -DKINOKO_PLATFORM_ONLY=ON -DCMAKE_BUILD_TYPE=Release -DKINOKO_RUNTIME_DIR="<absolute-new-runtime-directory>"
cmake --build build-runs/platform-01 --config Release --parallel 4
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_staged.ps1 -Name <unique-name> -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

Use a new build name and runtime directory for every batch. 每批使用新的构建名称与运行目录。

## Status / 状态

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7.

Windows x86 game: `kinoko_modern_gpu.exe` uses SDL window/input/audio/GPU.
`kinoko_retdec_rebuild.exe` retains D3D9 for comparison. The GPU game has no
D3D9/D3DX DLL imports; its own sources compile without D3D9 SDK headers.
Vendored SDL still includes unused D3D9 support. The user reports
`modern-gpu-game-02` currently normal; exhaustive runtime parity is not established.
Build with `tools/build_staged.ps1 -ModernOnly` (plus the usual arguments) to
omit legacy comparison targets. Full-game x64 and non-Windows support remain pending.
Keep the staged `shaders` directory and three DAT files beside the game EXE.
Portable modules: Windows x64, Linux, macOS build workflow; contracts compiled, not executed.

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).
