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
The default build uses only the modern game, with no D3D9/D3DX DLL imports,
SDK header dependency, SDL D3D9 renderer or D3D9 DLL loader. Vendored SDL has a
[documented local patch](../third_party/SDL3-3.4.16/KINOKO_PATCHES.md).
The old comparison target, SDK alias branches, cloned libraries and D3DX import
library have been removed. Active gameplay/VM contracts compile against the sole
modern backend. Old COM fixture sources are retained as non-build migration
evidence; their unported assertions are not counted as current test coverage.
The user reports `modern-width-01` currently normal; this is user feedback,
not agent-run validation. Full-game x64 and non-Windows support remain pending.
Keep the staged `shaders` directory and three DAT files beside the game EXE.
Portable modules: Windows x64, Linux, macOS build workflow; contracts compiled, not executed.

Font/atlas, shared string ownership and Squirrel object payload storage now
compile in isolated Windows x64 checks. ACT texture rendering uses a typed
state snapshot. The complete game still requires x86 ACT/VM layouts and method
adapters; these isolated checks do not establish x64 gameplay compatibility.
The user reports `modern-window-move-01` normal; this is user runtime feedback.
ACT textures/render targets now use native resource storage throughout creation,
clone, property I/O, script bindings, render access and cleanup. Retained clone
references live in each object; the pointer-keyed ownership table is removed.
Chip resources now use native storage and shared MCD ownership across cloning,
loading, map caches and script access. Texture/chip allocations also use native
array metadata. ACT calls use named typed interfaces with an explicit legacy
method-table adapter. The actual resource/string and method adapter contracts
compile independently for Windows x64; full-game x64, outer ACT layouts, mesh
storage and VM ABI migration remain pending. See [texture scope](../docs/modern-x64/act-texture-native.md)
and [chip/method scope](../docs/modern-x64/act-chip-methods.md).
The user reports `modern-act-native-02` normal; this is user runtime feedback.

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).

[Runtime graphics width audit / 图形运行时宽度审计](../docs/modern-x64/README.md).
