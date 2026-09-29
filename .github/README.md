# 6kinoko-modern

SDL3 modernization fork of 6kinoko-rebuild. Complete games now build for Windows x86/x64, Linux x86_64 and macOS Intel/Apple Silicon. Non-Windows packages are compiled and staged, but target-machine gameplay validation is still pending.

## Build / 构建

Windows, after committing source (use a fresh name each time):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_x64_probe.ps1 -Name <unique-name> -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

Linux/macOS full game (CMake 3.24+, C++17 compiler, SDL platform development dependencies, `glslangValidator`; macOS also needs `spirv-cross`):

```sh
cmake -S . -B build-runs/native-01 -DCMAKE_BUILD_TYPE=Release -DKINOKO_RUNTIME_DIR="<absolute-new-runtime-directory>"
cmake --build build-runs/native-01 --target kinoko_modern_gpu --parallel 4
```

For a universal macOS build, add `'-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'` when configuring. The CI workflow lists Linux development packages and uses `brew install glslang spirv-cross` on macOS. `KINOKO_PLATFORM_ONLY=ON` remains available for portable module builds. `tools/build_staged.ps1` retains the Windows x86 all-target build.

Always use new build/runtime directories. Copy your original `6kinoko_a.dat`, `6kinoko_b.dat` and `6kinoko_c.dat` beside the executable. Keep `shaders/` and `fonts/` beside it too. Game and contract execution remain with the user.

## Status / 状态

Apple Silicon follow-up: the user confirmed macOS starts, but reported striped
particles/doors and blurred digits (Windows looks normal). Source `32dc959a`
adds D24 precision compatibility on floating depth targets and Retina drawables.
All six [CI jobs](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36518686549)
passed; visual correction still requires user verification. See the
[rendering handoff](../docs/modern-platform/macos-rendering-01.md).

Source `6b39bfe4dfd8925003cc3d530ab91effcfc5af93` passed [all six CI jobs](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36460831199): complete Windows, Linux and universal macOS games plus portable compilation on all three OSes.

Local Windows delivery: `runtime-builds/modern-x64-native-05/kinoko_modern_gpu.exe`. The x64 game and selected contracts compiled; `modern-native-05` compiled the Win32 game and all 85 contracts. DAT hashes and static D3D9 audits passed. No game, CTest or contract executable was run.

Native CI artifacts `game-ubuntu-24.04` and `game-macos-14` contain `.tar.gz` packages with executable permissions, shaders, fonts, licenses and a hash manifest. They never include original DAT. Linux targets Ubuntu 24.04 x86_64 or compatible newer systems with Vulkan drivers. macOS targets Metal-capable macOS 14+ and contains both CPU architectures; the package is unsigned/not notarized. Extract to a writable directory and use `launch.sh` or `Launch.command`.

The user accepted the preceding `modern-x64-graphics-02` and earlier gameplay/save fixes. That feedback is not validation of the current font/backend changes or non-Windows runtime. Dynamic text remains available through the existing layout/atlas API, using portable CP932 decoding and bundled Noto Sans CJK JP. Font face names use this fallback; exact GDI metrics are not preserved. UTF-8 localization APIs are future work.

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7 · stb_truetype 1.26 · Noto Sans CJK JP. No game D3D9/D3DX backend remains. Preserve SDL's local patches. Original DAT/bytecode/save widths remain distinct from native pointers. Windows keeps its modal-drag hook and crash diagnostics; ordinary platform services use SDL or standard C++.

Next: target-machine launch/gameplay/save verification and any resulting platform fixes. Compilation alone does not establish runtime compatibility.

## Documentation / 文档

- [Full native game migration and delivery evidence](../docs/modern-platform/full-native-game.md)
- [Dependency provenance](../docs/modern-platform/dependencies.json)
- [Migration roadmap](../docs/modern-platform/TODO.md)
- [Historical platform foundation](../docs/modern-platform/README.md)
- [SDL GPU integration](../docs/modern-gpu/GAME.md)
- [Native-width history](../docs/modern-x64/BUILD.md)
