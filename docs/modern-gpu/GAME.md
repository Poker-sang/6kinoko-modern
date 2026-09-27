# SDL GPU game integration 

The full-game target is `kinoko_modern_gpu`. It uses the original DAT/VM/gameplay
sources, compiled against an audited source compatibility interface that records
SDL GPU commands. It does not link d3d9.lib or d3dx9_33.lib. The old
`kinoko_retdec_rebuild` target and its native-device contracts remain for comparison.
The SDK D3D9 enum/record definitions are still used at the compatibility boundary;
this is runtime replacement, not completion of the platform-header/x64 cleanup.

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
proof of visual parity. Build results and exact EXE/hash follow in BUILD.md.

The first full all-target build (modern-gpu-game-01, source 29ad5b46) succeeded.
Its new game import table contains neither d3d9.dll nor d3dx9_33.dll. Follow-up
hardening adds explicit recursive device-state locks, image snapshot locks,
first-use depth initialization and obsolete resize-depth resource reclamation.
The final batch is recorded separately; no runtime validation has occurred.
