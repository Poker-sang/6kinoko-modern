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
compile independently for Windows x64. Mesh resources now also own native
controller/render objects, with typed 3D drawing and a portable MSH/MAT decoder.
Full-game x64, outer ACT layouts, the mesh-manager ABI and VM bindings remain pending. See [texture scope](../docs/modern-x64/act-texture-native.md)
and [chip/method scope](../docs/modern-x64/act-chip-methods.md).
The user reports `modern-act-chip-02` normal; this is user runtime feedback.
See [mesh migration scope](../docs/modern-x64/mesh-native.md).

String layouts and glyph queues now use native C++ storage throughout factories,
ACT/script properties, cloning, rendering and destruction. Stable shared atlas
pages survive vector growth and source-layout destruction after text replication.
The native ownership contract compiles separately at Windows x64 (not executed).
The user reports `modern-mesh-native-01` normal; this is user runtime feedback.
See [string migration scope](../docs/modern-x64/string-native.md).

The next milestone is a complete Windows x64 game. An experimental full-game
compiler probe now records actual blockers without disabling source layout or
address guards. ACT/generic script calls pass explicit receiver/reserved slots
and typed floats at both widths. Serialized type hashes keep original 32-bit
results; crash diagnostics and locks compile with native Windows x64 layouts.
The complete x64 game is still blocked; isolated compile success is not gameplay.
The user reports `modern-string-native-01` normal. See the [full-game milestone](../docs/modern-x64/full-game.md).

Actor/camera script slots, callback ownership, pool allocation, animation addressing
and core container/reference records now use native pointer width. Seventeen real
implementation files and the new ownership contract compile at Windows x64;
runtime assertions were not executed. VM registration/property bridges and
ACT/map/application layouts still block the complete x64 game. The user reports
`modern-x64-entry-03` normal.

Common SqPlus class/property bindings, global closure signatures and ACT
document/layer/script/runtime ownership now use native widths. Actual binding,
ACT lifecycle and source VM value/GC implementations compile independently at
Windows x64; no runtime tests were executed. Remaining ACT publication slots,
map/input/application dependencies and bytecode-format compatibility still need
work before a complete playable x64 game. The user reports `modern-actor-native-03`
normal. See [binding/ACT scope](../docs/modern-x64/binding-act-native.md).

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).

[Runtime graphics width audit / 图形运行时宽度审计](../docs/modern-x64/README.md).
