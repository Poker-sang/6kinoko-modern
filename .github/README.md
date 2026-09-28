# 6kinoko-modern

SDL3 modernization fork of 6kinoko-rebuild, using SDL window/input/audio/GPU and updated Ogg/Vorbis. Complete Windows x86 and experimental x64 games now build; full Linux/macOS games remain future work.

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

The complete x64 game compiled and linked in `modern-full-x64-24`, source
`e720bc466dda17c600023c84a9cd3680da79ae3d`. Static inspection confirms AMD64 / PE32+.
EXE: `runtime-builds/modern-full-x64-24/kinoko_modern_gpu.exe`.
Three original DAT files were staged and size/SHA256 verified; two sprite shaders
are included. Keep the DAT files and `shaders` directory beside the EXE.

The same source built the Win32 game and 78 contracts in `modern-full-width-03`.
The new original-width bytecode contract also compiled separately for x64.
Both builds passed static D3D9 audits. No game, CTest or contract executable was
run. Experimental x64 runtime, real DAT bytecode loading and save compatibility
remain unvalidated; compilation does not prove all runtime pointer flows correct.
See [full-game delivery](../docs/modern-x64/full-game-native.md) and
[build instructions/evidence](../docs/modern-x64/full-game.md).

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7.

The sole modern game backend has no D3D9/D3DX imports, SDK headers, SDL D3D9
renderer or loader. SDL's [local patch](../third_party/SDL3-3.4.16/KINOKO_PATCHES.md)
must be preserved. Old COM fixtures remain non-build historical evidence.
Portable module CI covers Windows x64, Linux and macOS; it is not full-game
portability validation.

ACT/resources, actor/camera, map/collision, input/audio/application and VM bridges
now compile at native pointer width in the complete game. Native ownership and
typed method dispatch are separate from original fixed-width serialized hashes,
CV4 bytecode and save scalars. Compatibility implementation still needs runtime
verification. Historical migration scopes and build attempts remain in
[the x64 build record](../docs/modern-x64/BUILD.md).

Earlier user reports marked `modern-width-01`, `modern-window-move-01`,
`modern-act-chip-02`, `modern-mesh-native-01`, `modern-string-native-01`,
`modern-x64-entry-03` and `modern-actor-native-03` normal. These are user feedback
for earlier builds, not agent validation or acceptance of the new x64 executable.

Next: user x64 runtime acceptance and remaining Windows service migration
(including fonts) for complete Linux/macOS games.

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).

[Runtime graphics width audit / 图形运行时宽度审计](../docs/modern-x64/README.md).
