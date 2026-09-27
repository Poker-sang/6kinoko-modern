# 6kinoko-modern

6kinoko-rebuild 的现代化分支。本批接入 SDL3 窗口、物理输入与音频输出，升级 Ogg/Vorbis。完整游戏目前仍限 Windows x86；可移植模块单独构建，尚未验证游戏运行。

## Build / 构建

```sh
cmake -S . -B build-runs/platform-01 -DKINOKO_PLATFORM_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-runs/platform-01 --config Release --parallel 4
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_staged.ps1 -Name modern-01 -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

## Status / 状态

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7.

Windows x86 game: SDL window/input/audio + legacy D3D9.
Portable modules: Windows x64, Linux, macOS build workflow; contracts compiled, not executed.

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).
