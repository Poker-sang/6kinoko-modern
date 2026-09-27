# SDL GPU 2D backend foundation

Historical checkpoint: the full-game integration is now documented in [GAME.md](GAME.md).
The integration-work checklist below describes the earlier foundation, not current completion.

Decision: continue SDL GPU; no bgfx dependency is added. This checkpoint builds a
real GPU renderer and a separate Windows preview executable. The game still uses
D3D9: it is NOT yet selectable with an SDL GPU game option.

## Implemented

- SDL GPU device/window ownership, swapchain acquisition and ordered render passes.
- Offline HLSL -> DXBC SM5.1 shaders using Windows SDK 10.0.22621.0 x64 FXC by
  default. Compiler path and SHA256 are recorded in the build tree. Explicit
  `KINOKO_FXC`/`KINOKO_GPU_SHADER_SDK` overrides are supported and recorded.
- Sampled RGBA8 textures and RGBA8 offscreen targets, pitched uploads and actual
  texture ownership; target feedback and reads of undefined target contents rejected.
- Four legacy XYZRHW vertices expand to two ordered triangles with explicit ARGB
  conversion. The vertex shader compensates the existing -0.5 screen offset once.
- Alpha/additive/reverse-subtract/multiplicative blend equations with pipelines
  cached by full effective blend state and attachment format. Native alpha factors
  use alpha counterparts of color factors, as required by D3D12.
- Eight cached point/linear + wrap/clamp sampler combinations. One mip level only.
- One contiguous frame upload into geometrically grown, cycled GPU buffers.
  Texture transfer rows are 256-byte aligned for D3D12; no per-sprite buffer allocation.
  Ordered draw calls remain separate; draw merging is a later optimization.
- Window pixel extent is reacquired each frame. Logical extents optionally scale
  the scene; minimized/no-swapchain frames skip rendering. No draw sorting.
- Initial validation and resource preparation precede command submission. GPU
  resources are released through SDL, which tracks in-flight use; final shutdown waits.

## Manual preview (not run by the agent)

Launch `kinoko_gpu_preview.exe` beside its `shaders` directory. No game DAT is needed
by this synthetic preview. Escape closes it; resizing scales the fixed logical view.
Top row: offscreen composition, linear sampling, repeated texture. Bottom row:
alpha, additive, reverse subtract, source-color multiply and destination-color add.
Shader compilation and C++ builds do not establish that these render correctly on
any GPU. Debug-layer/readback/visual validation remains unexecuted.

The renderer requires calls on the window/renderer owner thread. The actual game
currently renders on a worker while its window belongs to the main thread. The
integration batch must transfer immutable frame commands to the presentation thread
with texture lifetime/order synchronization; do not acquire the swapchain on the
worker or run two graphics APIs against the same window.

## Integration work remaining

1. Replace the legacy COM texture registry boundary and all sprite/target consumers
   together, preserving legacy handles, metadata, source/allocation dimensions and
   pixel conversion from ARGB1555/ARGB8888 into the backend's RGBA8 uploads.
2. Snapshot complete effective blend/sampler state and transfer ordered frames from
   the game thread to the main thread. Define shutdown, reset and upload handoff.
3. Add depth/cull/alpha-test state, mesh transforms/indexed buffers and the distinct
   0x4142 vertex path. None are silently represented by the 2D preview.
4. Add an explicit game backend option only when unsupported calls are detected and
   reported rather than ignored. Keep D3D9 as the default comparison path until user
   parity validation. Do not describe this preview as a working SDL GPU game.
5. Provide pinned SPIR-V/Metal shader artifacts and validate GPU operation on those
   systems. Portable C++ compilation alone is not Linux/macOS rendering support.

User reports modern-render-03 currently normal; this is user feedback only.
No game or test executable is run during this batch. CPU vertex contracts compile;
GPU execution and original-DAT parity are not claimed.
