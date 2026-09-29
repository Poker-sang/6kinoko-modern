# Apple Silicon rendering correction (2026-09-29)

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
