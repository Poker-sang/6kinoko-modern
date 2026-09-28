# Full native game migration (2026-09-29)

User requests continuing until non-Windows platforms can run the complete game.
User accepted modern-x64-graphics-02. Most visible text is believed to be image
assets; preserve dynamic text for future localization rather than deleting it.

Work in this batch:
- Enable full native Linux/macOS targets and CI; retain Windows x86/x64 builds.
- SDL startup and error dialogs; native move/resize hook remains Windows-only
  because that OS modal loop blocks ordinary SDL pumping.
- Portable compiler conventions preserve explicit receiver/unused arguments.
- Fixed-width status/state values, standard atomics/formatting/bounded copies.
- Portable CP932 character traversal and generated Unicode map for original DAT.
- Shared stb_truetype + OFL Noto Sans CJK JP font assets; no system font discovery
  or GDI dependency. Original face names use the bundled fallback, so glyph
  metrics can differ. Text layout/atlas ownership APIs remain available.
- GLSL compiled offline to SPIR-V for Vulkan and MSL for Metal; retain Windows
  DXBC. The game stages and selects the appropriate shader format.
- Portable floating rounding, path splitting and diagnostics. Windows crash
  capture stays native; other platforms retain traces without pretending to
  provide Windows SEH/minidumps. VirtualQuery address validator had no caller.
- Remove unused process context/native-window slots. Multiple instances are no
  longer rejected through a Windows-only named mutex.

All builds must record committed source, use fresh directories and retain logs.
No game or contract is executed by the agent. CI packages must not redistribute
original game DAT; users copy their three DAT beside the packaged executable.
Full build success alone does not establish gameplay correctness.

The legacy missing-length string scanner retains its Windows VirtualQuery guard
for historical invalid-pointer contracts. Native portable callers require valid
terminated string storage and retain the one-MiB cap; arbitrary address probing
has no standard/SDL equivalent. This is an explicit legacy compatibility boundary.
SqPlus release hooks use SQInteger return width on LP64, matching SQRELEASEHOOK.

## Delivery evidence

Built source: `6b39bfe4dfd8925003cc3d530ab91effcfc5af93`.
[CI run 36460831199](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36460831199)
passed all six jobs: full Windows/Linux/universal macOS games and portable
compilation on Windows/Linux/macOS. Later documentation or font-investigation
commits are not the source of these binaries.

Local Windows x64: `runtime-builds/modern-x64-native-05/kinoko_modern_gpu.exe`.
The game and selected text-encoding/GPU-resource/application contracts compiled.
Win32 `modern-native-05` compiled the game and all 85 contracts. Both deliveries
passed DAT hash checks and static D3D9 dependency audits. The x64 executable
SHA256 is `8500baa4f5f2af6ffedd99b8237026cd80044d76b84f892c5c35319026906f4d`.
Build logs and audit JSON remain in their respective `build-runs/` directories.

Downloaded native packages passed their original manifest hash checks. Local
copies of the three user-owned DAT were staged and hash-verified, and executable
permissions were preserved in these local-only delivery archives:

- Linux: `runtime-builds/modern-linux-native-01/6kinoko-modern-linux-x64-6b39bfe4-with-data.tar.gz`
  (104969200 bytes), SHA256
  `1e4e06d4245b983a9355cd14e91354cbb567c221dd39e6c4562bbec7f02fc51d`.
- macOS: `runtime-builds/modern-macos-native-01/6kinoko-modern-macos-universal-6b39bfe4-with-data.tar.gz`
  (106065242 bytes), SHA256
  `10dfa515c6bed5e603985989be11b9bed7d558bc0f714a06f567adc8f4b42598`.

These DAT-containing archives are not uploaded to GitHub. Original data-free CI
archives and manifests remain available locally. Download/build/dependency
evidence is retained in `build-runs/native-ci-6b39bfe4-linux/` and
`build-runs/native-ci-6b39bfe4-macos/`.

Extract the entire archive on the target machine into a writable directory.
Linux requires Ubuntu 24.04 x86_64 or a compatible newer system with Vulkan
drivers; run `./launch.sh`. macOS requires Metal-capable macOS 14+; launch
`Launch.command`. Its executable contains Intel and Apple Silicon slices and
uses system frameworks/libraries, without Homebrew dylib dependencies. The
macOS package is unsigned and not notarized.

No game, CTest or contract executable was run by the agent. Target-machine
startup, gameplay and save/load verification remain pending with the user.
Portable dynamic text is retained, but bundled font metrics may differ from
GDI; a future explicit UTF-8 localization API remains on the roadmap.
