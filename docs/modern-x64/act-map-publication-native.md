# ACT publication, map and collision native-width checkpoint

## Implemented scope

- ACT class/property/instance publication uses complete HSQOBJECT byte storage,
  including globals, locals and embedded layer slots. Payloads use memcpy-based
  pair readers/writers, not word 1 (padding on x64). Environment-to-script keys
  use intptr_t; reference acquisition/release order is preserved.
- All migrated native closure entry points return SQInteger-compatible intptr_t.
  Game scalar arguments still use int32_t and convert from SQInteger through a
  native temporary. No eight-byte VM write targets a four-byte game local.
- 2D/3D/map property publishers and Init/OnCreate callback selection use native
  offsetof values. Script/resource publication prefixes tie to canonical types.
  The runtime active-holder allocation no longer returns an integer address.
- Map creation and loading now use the same LayoutRecord as drawing/cloning;
  the duplicate MapLayoutRecord factory schema is removed. Native quad and
  vector storage expand with pointer width. Layer access is asserted against
  LayerAssociationRecord/LayerStorageRecord. Placement and ChipDefinition
  remain 32/48 bytes, preserving their scalar/file data contracts.
- Map clone copies named scalar/pointer fields, preserves virtual identities,
  clones native-sized element spans, resets sprite identities, and leaves the
  original reserved words unassigned. Two dormant legacy render streams have
  no active producers; their opaque 4/12-byte payloads are retained, while their
  begin/end/owner records use native widths. They are not inferred as live pointers.
- Map manager embeds native SqPlus storage. Collision state, hit records and
  actor weak references use pointer-width storage; point-query scratch now
  reserves a full StateRecord. Span/count and high-water calculations no longer
  truncate addresses. Collision math, slope/platform rules and weak-ref order
  are unchanged. Map rendering uses the explicit reserved-argument method bridge.
- The obsolete 457A10 mesh-child manager had no production caller or virtual
  table entry: references were confined to its adapter and a stub-only adapter
  test. Its global was only initialized to null. Remove that dead chain and its
  stub check; active ResourceState/Controller ownership and rendering remain.
  Source evidence and old implementation remain in Git history and decompiled
  reference. Mesh type-name hashing now receives direct character pointers.

## Compile evidence and limits

`modern-act-map-x64-06`, source `28eb3e9c`, compiles 47 production translation
units plus one layout/signature assertion unit. This includes every reconstructed
act_*.cpp, act_binding.cpp, actual map/collision and actor motion code, text
render consumers, camera/map registration and Sqrat/native property bridges.
It is an object build, not a linked or playable x64 game. Checks were not executed.
The original x86-only ref-watch walker remains a diagnostic boundary: x64 keeps
native event/name logging without passing truncated addresses to that walker.

Full Win32 staging (`modern-act-map-02`, 77 contracts compiled) and full-game
x64 probe (`modern-full-x64-15`, still blocked) are recorded in BUILD.md.
D3D9 static audit passed; three DAT were staged and verified.
All failed/intermediate attempts are retained. Earlier attempts exposed a map
method-table pointer type, an unused legacy header dependency, one missed VM
array, and a template allocation alias; these are fixed in the final source.

The next main work is the remaining host/input/VM/file interfaces. Original
compiled DAT bytecode/save numeric compatibility and runtime ACT behavior still
need verification after full x64 linking. Native compilation does not establish
that these runtime obligations are complete.
