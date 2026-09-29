# 6kinoko-modern

SDL3 modernization fork of 6kinoko-rebuild. Full games build for Windows x86/x64, Linux x86_64 and macOS Intel/Apple Silicon. Windows and Apple Silicon rendering have user validation; the user also confirmed block stars, balloon stars and floating stomp scores in Linux/WSL build `067fc414`. Full gameplay/save acceptance remains separate from automated tests.

## Build / 构建

Windows, after committing source (use a fresh name each time):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_x64_probe.ps1 -Name <unique-name> -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

Linux/macOS require CMake 3.24+, a C++17 compiler, SDL development dependencies and `glslangValidator`; macOS also needs `spirv-cross`:

```sh
cmake -S . -B build-runs/native-01 -DCMAKE_BUILD_TYPE=Release -DKINOKO_RETDEC_DISABLE_TRACE=ON -DKINOKO_RUNTIME_DIR="<absolute-new-runtime-directory>"
cmake --build build-runs/native-01 --target kinoko_modern_gpu --parallel 4
```

For universal macOS add `'-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'`. CI lists Linux dependencies and macOS shader tools. `KINOKO_PLATFORM_ONLY=ON` builds portable modules; `tools/build_staged.ps1` retains the Windows x86 build. Always use fresh build/runtime directories and retain artifacts.

## Run / 运行

Extract the complete package to a writable directory. Put your original `6kinoko_a.dat`, `6kinoko_b.dat`, `6kinoko_c.dat` beside the executable, preserving these exact filenames. Keep `shaders/` and `fonts/`. CI packages never include proprietary DAT.

| Platform | Normal launch | Diagnostic launch |
| --- | --- | --- |
| Windows | `launch.cmd` | `run-with-diagnostics.cmd` |
| Linux | `./launch.sh` | `./diagnose.sh` |
| macOS | `Launch.command` | `Diagnose.command` |

Normal launch explicitly disables tracing. Diagnostic launch enables low-volume logs beside the executable: `retdec_trace.log` on Windows, `kinoko-trace.log` elsewhere. Existing logs append and are preserved. For targeted native effect diagnostics, run the executable directly with `KINOKO_TRACE=1 KINOKO_TRACE_VERBOSE=1 KINOKO_TRACE_FILTER=star`.

Linux targets Ubuntu 24.04 x86_64 or compatible newer systems with Vulkan. macOS targets Metal-capable macOS 14+, contains both CPU slices, and is unsigned/not notarized. WSL2 can run the Linux package through WSLg; this machine currently uses CPU Vulkan (llvmpipe). See the WSL record before interpreting performance. If the WSLg COPY MODE issue recurs, restarting WSL can restore the window; `wsl --shutdown` stops all WSL processes.

## Status / 状态

Script random numbers now preserve the original Windows CRT sequence and 0–32767 range on all platforms. Metal honors row/slice pitch; mixed-zero defaults now match D3D12/Vulkan independently. No game shaders, physics or asset dimensions were altered to hide these issues.

Full-game CI runs focused non-gameplay regressions for original bytecode/script numerics, save roundtrips and malformed files, archive lookup/decoding, random sequences, clocks/synchronization and directory matching. Logs and retained save/archive fixtures ship in CI artifacts. The GPU upload/download contract requires a graphics backend and runs separately. Automated tests do not establish that a level can be cleared or that real gameplay saves reload correctly.

Packages use the same source revision and include hash manifests, shaders, fonts and licenses. Windows artifacts are `windows-game-Win32` / `windows-game-x64`; native artifacts are `game-ubuntu-24.04` / `game-macos-14`. Latest build results and local paths are in the closeout record below.

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7 · stb_truetype 1.26 · Noto Sans CJK JP. D3D9/D3DX is removed. Preserve local SDL and Squirrel patches when updating. DAT/bytecode/save widths remain separate from native pointer sizes. Windows retains its modal-drag and crash adapters; ordinary services use SDL/C++. Dynamic text is retained for future localization; bundled fonts do not exactly reproduce GDI metrics.

Next: user verification of level entry/completion, effects and save/quit/reload on each platform; then input action separation and the shared input layer before TAS/MOD expansion.

## Documentation / 文档

- [Cross-platform closeout, tests and packages](../docs/modern-platform/cross-platform-closeout.md)
- [Star/score fix and evidence](../docs/modern-platform/linux-effects-01.md)
- [WSL setup and runtime evidence](../docs/modern-platform/wsl-validation-02.md)
- [Full native migration history](../docs/modern-platform/full-native-game.md)
- [Dependency provenance](../docs/modern-platform/dependencies.json)
- [Migration roadmap](../docs/modern-platform/TODO.md)

Independent action bindings preserve keyconfig.dat: [input actions](../docs/modern-platform/input-actions.md). Existing settings UI is unchanged.
