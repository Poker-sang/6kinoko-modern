# Rendering checkpoint 2: textures, quads and scoped state

The active renderer is still D3D9. This batch connects the existing production
paths to backend boundaries; it does not add an SDL GPU device or shaders.
User reports modern-render-02 currently normal. This is user feedback only.

## Production changes

- `render_resources.hpp` defines portable texture descriptions, signed native-width
  mapped row pitch, texture mapping/creation, quad submission and state-scope kinds.
  `sprite_vertex.h` retains the 28-byte ARGB/UV vertex without calling-convention or
  Windows headers. No native pointer is represented by an integer handle.
- `texture_pixels.hpp` receives a borrowed CV2 pixel view, independent of the x86
  bitmap object layout. Raw and RLE copying retains row pitch, palette behavior,
  cross-row runs and the original raw-16 odd-width partial copy.
- `D3D9Texture` owns one reference and adapts sampled/render-target creation,
  mapping and unmapping. Creation stays inside the existing graphics lock.
  Source dimensions remain distinct from allocation dimensions. The legacy output
  transfers ownership through detach, including the original nonzero-lock-result
  behavior. Invalid mappings still unlock/release; unlock status remains ignored.
- The portable binding cache preserves repeat-handle returns, unconditional zero
  unbind, failed-bind caching and final-release invalidation. The D3D adapter alone
  resolves handles to native bindings. Validation still occurs before cache lookup.
- Final store retirement queries actual native stages (not just cached handles),
  releases GetTexture references, unbinds matching stages, invalidates cached handles,
  releases the owned texture and then clears registry metadata, in the old order.
- Sprite and quad paths use `D3D9QuadSink`; the original vertex storage, strip order,
  four vertices, stride, half-pixel coordinates and return values remain unchanged.
  Setup failure does not suppress drawing. 404BC0's distinct 0x4142 FVF is retained;
  it must be audited explicitly when implementing shaders, not silently normalized
  to the ordinary screen-space format.
- `D3D9DrawState` centralizes three existing state scopes: blend-only for text/2D
  layouts, wrap-only for maps, and clamp-plus-blend for the ACT pass. Snapshot
  Get/Set order, failed-Get initial values, and restoration order are preserved.
  Native values remain private to the snapshot so unfamiliar states round-trip.
  Explicit restoration is idempotent; the destructor restores on scope exit too.
  Central cached blend state is deliberately not synchronized on restoration,
  matching existing behavior. Filtering/depth state is not newly restored.
- ACT frame blend writes now use the semantic blend adapter, preserving its own
  source/destination/operation order.

## Verification boundaries

The new portable resource contract covers failed bind caching, zero unbind,
cache invalidation, quad setup-failure/draw continuation, original vertex pointer,
pitched upload, odd-width raw16, cross-row RLE, truncated runs and palette upload.
The existing texture image/store, sprite/quad, ACT/layout, map and device lifecycle
contracts continue to compile the production implementation and fake native devices.
Compile-only CI includes the portable contract on Windows x64, Linux and macOS.
No test executable or game is run by the agent. A successful compilation is not a
runtime parity result. Exact build source/hash is recorded in BUILD.md and manifest.

## Remaining boundary before an SDL GPU game

The legacy registry and C image-loading entrypoint still expose COM pointers for
mesh/render-target consumers. They are explicit compatibility boundaries, not a
portable registry. GPU integration must replace those consumers with owning backend
resources together, preserving public integer resource IDs and metadata. Native
backbuffer/surface selection, reset listeners, mesh buffers/transforms, central
alpha/depth/filter state and window/device lifetime remain D3D9. No x64 game claim.

Next: build the SDL GPU device/shader/resource registry and sprite render-target
path behind a backend option on Windows. Keep the D3D9 baseline available; cover
mesh/depth/alpha and audit 0x4142 before selecting the new backend by default.
