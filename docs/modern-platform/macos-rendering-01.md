# Apple Silicon rendering correction (2026-09-29)

**Rejected / reverted:** the user reported this candidate still renders badly.
Implementation reverted in `444faba`; retain this file as historical evidence,
not a current fix. Video inspection subsequently identified a concrete Metal
texture upload stride defect. See `macos-rendering-02.md`.

User feedback on native source `6b39bfe4`: macOS on an M-series Mac starts,
but chimney particles and the animated door show triangular/striped artifacts,
and level digits look blurred. The user reports Windows renders normally.
This establishes macOS startup, not complete gameplay/save compatibility.

Two backend differences were found in source inspection:

- The original presentation requests D24S8. Apple GPUs reject this format, so
  the shared renderer falls back to D32F/S8. Previously it used the floating
  depth values directly, changing the precision and equality behavior of
  overlapping legacy layers. Fragment shaders now round depth to the D24 UNORM
  grid on this fallback before depth comparison/storage. Native D24 uses the
  original fragment depth. HLSL/GLSL interfaces retain the same 16-byte block.
- The SDL window did not request HIGH_PIXEL_DENSITY. Cocoa therefore made a
  low-resolution Metal drawable and the compositor enlarged it on Retina.
  Request physical-pixel drawables through SDL while retaining game logical
  dimensions. Existing pass extents handle the coordinate conversion; texture
  filtering and game draw order remain controlled by the existing render state.

Depth-format evidence: `src/reconstructed/device_initialization.cpp`,
`src/platform/sdl_gpu_renderer.cpp`, and the vendored SDL Metal format query.
Retina evidence: `src/platform/sdl_platform.cpp` and SDL's
`src/video/cocoa/SDL_cocoametalview.m` HIGH_PIXEL_DENSITY handling.

The striped-artifact diagnosis is a hypothesis consistent with the screenshot
and format difference, not a confirmed GPU capture. This correction requires
user verification on the affected Mac. No game or contract execution by the
agent. Compile shaders and complete packages before handing off. Preserve all
earlier packages and saves; stage only the three reference DAT in new packages.

## Build and delivery

Built source: `32dc959ae77d3cd7bbd19a1e3e8c74fd95005e4f`.
[CI 36518686549](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36518686549)
passed all six Windows/Linux/macOS full-game and portable jobs. Shader sources
compiled through FXC and glslang/SPIRV-Cross. Generated MSL retains explicit
fragment depth output and the D24 rounding operation.

Local Windows x64 `runtime-builds/modern-x64-macos-render-01/kinoko_modern_gpu.exe`
compiled and passed DAT hash validation and the static D3D9 dependency audit.
No game or contract was executed.

macOS universal local package, including hash-verified user-owned DAT:
`runtime-builds/modern-macos-render-01/6kinoko-modern-macos-universal-32dc959a-with-data.tar.gz`.
SHA256: `78b049f261edac36382f8934062e08811e8d8c28c94a2e5b1b4c3a2ea601b0ff`.
Original CI package manifest and executable permissions were verified. Download,
build logs, staging script and delivery record are retained under
`build-runs/macos-render-delivery-01/`. DAT packages remain local only.

Extract the complete new package and open `Launch.command`. Observe chimney
particles, door animation and level digits on the affected M-series Mac. Actual
visual resolution remains unverified; do not treat compilation as confirmation
of the depth hypothesis or as a gameplay test.
