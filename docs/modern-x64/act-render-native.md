# ACT rendering native-width checkpoint

The ACT rendering/lifetime priority is in progress, not completely closed.

- Layout2DRecord and Layout3DRecord use native pointer alignment. Historical x86
  assertions remain, alongside native assertions for the contiguous 17 property
  words and world matrix. Allocations/clones use sizeof/offsetof.
- 2D and 3D owner access now uses LayerStorageRecord and LayerAssociationRecord,
  the same schemas as construction and destruction. Visibility tests the low
  byte only; debugOnly does not make an invisible layer visible.
- Remove the real 32-bit allocation-address roundtrip in the 2D clone and the
  truncated constructor-result check in its factory. Preserve constructor
  virtual identities and the original selective clone assignment.
- BlitCommand remains 36 scalar bytes; embedded sprites use native alignment.
  Drawing/cache copy/resize/iteration use the same BlitSprite schema.
- ACT array/list pointer records use native size; lifecycle operations and name
  diagnostics no longer depend on the legacy integer-address helper.
- Preserve rotation/translation/scaling order, low-byte colors, reverse layer
  traversal, resource suspension order, borrowed references, and clock overflow.
- Type-name scratch storage follows StringRecord size/alignment; property stream
  integers/floats and archive formats are unchanged.

`modern-act-render-x64-02` compiled 11 production translation units at source
`1dcadbc3` with target `kinoko_act_render_width_compile`. The first attempt is
retained and failed due to a declaration preprocessor error, corrected in 02.
No game or test executables were run. Full-game compilation evidence follows
in BUILD.md after the staged build and full x64 probe.

## Remaining ACT work at this historical checkpoint

The native publication/map follow-up is now implemented and compiled; see
[ACT/map checkpoint](act-map-publication-native.md). The list below records the
prior handoff, not the current remaining migration scope.

1. The large act_binding.cpp still has original two-word local VM slots,
   integer callback addresses and scalar property offsets. Migrate storage,
   signatures and publishers together; do not just disable architecture guards.
2. Map layouts duplicate factory and renderer schemas. Native map vectors,
   clone ranges, manager script objects and collision references need a coherent
   migration. The map_layer prefix is not yet a native ACT ownership schema.
3. The mesh-manager child ABI remains legacy; do not infer its unknown prefix
   from the already-native mesh resource controller.
4. Compile the complete ACT/map/script graph at x64, then verify bytecode/save
   numeric compatibility and user gameplay after the full host is available.

The isolated rendering build does not cover those remaining paths and does not
establish a runnable x64 game or a completed ACT migration.
