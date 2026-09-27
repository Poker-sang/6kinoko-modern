# Rendering migration checkpoint 1

Follow-up: [checkpoint 2: resources and quad submission](RESOURCES.md).
The remaining-boundary statements below describe checkpoint 1.

## Decision: SDL GPU

Use SDL GPU for the replacement renderer. Keep D3D9 as the active Windows x86
baseline until the replacement covers the following requirements. SDL Renderer
is convenient for 2D images but does not expose the explicit 3D vertex streams,
depth/stencil pipelines and shaders required by the existing Mesh/C3D path.
Do not silently drop that path even though its use in ordinary gameplay is unverified.

The vendored SDL 3.4.16 `include/SDL3/SDL_gpu.h` exposes reverse-subtract blend,
vertex buffers, indexed draws, render targets and depth/stencil pipeline state.
GPU backends use Vulkan / D3D12 / Metal; shader formats must be supplied for each
selected backend (SPIR-V, DXIL/DXBC as supported, Metal). Pin the offline shader
compiler and commit reproducible shader sources/artifacts in the implementation
batch. SDL GPU has no fixed-function D3D9 emulation: alpha test, texture modulation,
transforms and transformed vertices need explicit shaders. Hardware support and
format availability must be checked; this decision does not establish old-GPU support.

## Requirement and remaining-boundary inventory

| Area | Current production evidence | Replacement requirement |
| --- | --- | --- |
| Sprites | `quad_submit.cpp`: XYZRHW, ARGB diffuse, UV, 28-byte vertices, four-vertex strip | Preserve order, color channel conversion and sampling; translate the D3D9 half-pixel rule deliberately, not twice. Original x/y subtract 0.5, z adds 0.5. |
| Blend | `render_state.cpp`, `act_render_state.cpp`, frame/text save-restore | Add, reverse subtract, source alpha, inverse source alpha, zero, one, source/destination color. Build pipelines from complete effective state after partial writes. |
| Sampling | `render_state.cpp`, `act_map.cpp`, `act_frame_render.cpp` | Point/linear MAG/MIN/MIP, wrap/clamp U/V and scoped restoration. |
| Textures | `texture_image.cpp`, `texture_store.cpp`, `texture_binding.cpp` | Preserve allocation versus image size, row pitch, CPU upload and handle lifetime; remove COM texture slots only when all consumers migrate. |
| Targets | `render_target.cpp`, `renderer.cpp` | A8R8G8B8 render targets, backbuffer restoration, color/depth/stencil clears; reacquire borrowed/acquired surfaces at reset boundaries. |
| Depth/alpha/cull | `render_state.cpp`, `act_layout_3d.cpp`, `device_initialization.cpp` | Depth test/write, compare functions, alpha reference/test in shader, winding and D24S8-equivalent attachment support. |
| Mesh | `act_mesh.cpp` | Position/normal/UV streams, 16-bit uploaded indices, indexed triangle lists; retain material ordering and the existing final debug triangle until separately authorized. |
| Matrices | `act_layout_3d.cpp`, `camera_projection.cpp` | Preserve yaw/pitch/roll -> translation -> scaling multiplication and camera floor/ceil; audit handedness, matrix layout and clip-depth conversion before replacing D3DX. |
| Clipping | No explicit SetScissorRect/SetViewport call found in reconstructed cpp search | Do not invent scissor behavior; still audit CPU clipping and target-dependent viewport/default raster clipping when migrating draws. |
| Lifetime | `device_initialization.cpp`, `device_runtime.cpp`, `render_target.cpp` | Scene/present order, reset listeners, window changes and resource recreation. SDL command-buffer lifetimes differ from COM reset semantics. |

Stencil attachment creation and clears are confirmed; actual stencil-tested drawing
is not established by this audit. Mesh historical evidence is preserved under
`docs/mesh-3d-layouts/`; this batch reads the existing reconstruction/evidence and
does not claim fresh original-binary analysis or new runtime observations.

## Implemented boundary

`include/kinoko/render_blend.hpp` is a Windows-free semantic command interface and
shared transition algorithm used by the real central and ACT blend paths.
`src/reconstructed/d3d9_blend_sink.hpp` is a borrowed-device adapter that translates
semantic enums to D3D9. No native pointer is stored in a 32-bit integer. Cache layout
and the renderer's legacy ABI remain unchanged.

Preserved details: unchanged is distinct from the zero factor; writes remain ordered;
earlier failures do not suppress later writes; the last result is returned; repeated
central modes return the mode; unsupported transitions still update the cache;
ACT case 32 omits the operation write; ACT out-of-range modes use the graphics
device and do not update the central cache. Transition-key arithmetic now uses
64-bit integers so arbitrary public mode inputs cannot cause signed overflow.

The portable contract covers call order, partial updates, error propagation, no-op,
null sink, unsupported mode and ACT divergence. Existing fake-D3D contracts still
compile the production adapters. Tests are compiled only, not executed.

This is a first blend-state boundary, not a completed renderer abstraction.
Quad submission, other state setters, scoped Get/Set state consumers, textures,
meshes and lifecycle still call D3D9 directly. An SDL GPU sink must track complete
state, not emit a new immutable pipeline for every legacy partial write.

## Next implementation batches

1. Extract typed texture ownership/upload and sprite submission, including all scoped
   blend/sampler restoration; keep D3D9 runnable as the comparison backend.
2. Add SDL GPU device, pinned shaders and Windows sprite/target path behind an
   explicit backend option. Complete depth/alpha/mesh paths before switching default.
3. Replace D3DX math/texture helpers and remove D3D9 after user parity validation.
4. Continue Win32 services and x64 layout migration; then full Linux/macOS games.

User reported `modern-audio-03` currently normal. This is user feedback, not agent-run
validation of every audio edge case. No game or tests are run in this batch.
