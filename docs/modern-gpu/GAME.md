# SDL GPU game integration

The full-game target is `kinoko_modern_gpu`. It uses the original DAT/VM/gameplay
sources, compiled against explicit project-owned graphics interfaces that record
SDL GPU commands. It does not link d3d9.lib or d3dx9_33.lib. The old
`kinoko_retdec_rebuild` target and its native-device contracts remain for comparison.
The modern game no longer includes SDK D3D9 enums/records or rewrites SDK class
names with preprocessor macros. `graphics_api.hpp` selects explicit backend types;
`graphics_types.hpp` owns modern command values and descriptions. Only the native
comparison branch aliases SDK types. Modern target include guards reject accidental
D3D9 header use. `KinokoGraphics` is a runtime object; its modern capabilities
description contains only the fields used by the game, not the SDK 304-byte layout.
Command values remain compatible with existing numeric game states. A comparison-only
compile contract checks them against the SDK, plus matrix/vertex/map layouts.

The API still retains legacy command methods and Windows status/window types.
Renderer records and other original x86 object layouts still need migration;
this is not a full-game x64 or Linux/macOS port.

Use `tools/build_staged.ps1 -ModernOnly` with the usual name/resource/generator
arguments to build and stage only the modern game and its required dependencies.
This sets `KINOKO_BUILD_LEGACY_COMPARISON=OFF`, excludes original root targets
from ALL unless required by the modern dependency graph, and disables their CTest
entries. Explicit comparison targets remain available; the default remains ON.

The new path covers texture creation, CV2 ARGB1555/ARGB8888 conversion, mutable
font atlas snapshots, binding, render targets, source/allocation dimensions,
ordered quad/triangle submission, mesh CPU buffers and indices, world/view/projection,
depth test/write/compare, culling, shader alpha test, and blend/sampler restoration.
XYZW (0x4142) is distinct from screen XYZRHW; it uses homogeneous positions and
matrix transformation. Default mesh lighting has no enabled lights/ambient and
therefore black RGB; unimplemented state requests are reported as errors.

GPU texture storage is owned by the main presentation thread. Each queued frame
owns the image and pixel generation it sampled; changing a font atlas or retiring
a scene cannot replace queued pixel data. Old GPU generations are reclaimed after
no queued/current image uses them. SDL defers physical destruction while in flight.
BeginScene throttles when three complete frames are queued, without blocking while
holding the legacy graphics lock. EndScene publishes immutable passes. The old
Present entry acknowledges submission; the window thread actually presents while
polling. Errors latch and are shown on the window thread, followed by orderly quit.
When minimized, offscreen passes still execute so target history is not dropped.

Texture creation and CPU mapping no longer invoke D3DX. Matrix helpers use portable
C++ row-vector math with the original rotation -> translation -> scaling order.
Floating-point bitwise equivalence to D3DX is not asserted. Depth storage prefers D24S8 and explicitly falls back to
D32_FLOAT_S8_UINT where necessary; depth precision and render-target transitions
need user visual validation. Per-extent depth targets replace the legacy shared
backbuffer depth surface; cross-extent offscreen mesh depth sharing is not proven.

No game or local tests are run. A successful build and a DLL-import audit are not
proof of visual parity. Build results and exact EXE/hash are recorded in BUILD.md.

The first full all-target build (modern-gpu-game-01, source 29ad5b46) succeeded.
Its new game import table contains neither d3d9.dll nor d3dx9_33.dll. Follow-up
hardening adds explicit recursive device-state locks, image snapshot locks,
first-use depth initialization and obsolete resize-depth resource reclamation.
The final batch is recorded separately. The user reports modern-gpu-game-02
currently normal; this is user feedback, not agent execution or exhaustive parity
validation. Prior full-game CI run 36322559102 completed successfully.
