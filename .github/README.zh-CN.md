# 6kinoko-modern

6kinoko-rebuild 的现代化分支。本批接入 SDL3 窗口、物理输入与音频输出，升级 Ogg/Vorbis。完整游戏目前仍限 Windows x86；可移植模块单独构建，尚未验证游戏运行。

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
D3D9/D3DX DLL imports or SDK header dependency. 用户反馈
`modern-gpu-game-02` 目前正常，尚未完成全面运行一致性验证。
使用 `tools/build_staged.ps1 -ModernOnly`（加上常规参数）可跳过旧版对照目标。
完整游戏的 x64 和非 Windows 平台迁移仍待完成。
Keep the staged `shaders` directory and three DAT files beside the game EXE.
Portable modules: Windows x64, Linux, macOS build workflow; contracts compiled, not executed.

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).
