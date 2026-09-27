# x64 preparation: runtime graphics records

This batch prepares selected graphics runtime objects; the complete game remains
Windows x86. The user reports modern-width-01 currently normal. That is user
feedback, not agent-run validation.

## Completed boundary

- `graphics_runtime.hpp` contains native renderer, texture-slot and listener
  records. They contain real pointers and have no original x86 size/offset contract.
- The renderer no longer has a fake vtable prefix, unused original padding or a
  100-byte size assertion. Its runtime bootstrap no longer receives a renderer
  method table. No DAT/ACT file layout was changed.
- Device notifications use an embedded listener with explicit context and function
  pointers. No cast between the renderer and an object-prefix virtual interface,
  integer address, or mismatched thiscall/fastcall callback is needed.
- Registration still borrows listeners, preserves insertion order, suppresses
  duplicate registrations and uses the existing graphics lock. Reset still releases
  acquired surfaces before reacquisition, with original slot/return semantics.
- Texture slots already used actual pointers. Their declarations now live beside
  the runtime renderer, and compile with native pointer width. Texture handles
  remain signed 32-bit registry indices (capacity 4096), not encoded addresses.
  Source/allocation dimensions, lookup, reference counts and release order are unchanged.
- The same runtime/listener contract is compiled by the portable CI matrix. On
  Windows it also includes the real renderer/texture public headers. Runtime
  assertions cover callback context, order, duplicate suppression, removal and
  borrowed lifetime, but have not been executed.

## Width audit and triage

`pointer-width-candidates.json` records maintained-source review candidates at
commit 686d54ba. Reproduce with `python tools/audit_pointer_width.py --output <path>`.
The scan covers 367 source/header/CMake files, excluding original decompilation,
third-party trees, historical evidence and tests. It found 114 pointer-to-int32
cast candidates, 462 legacy address-boundary candidates, 29 x86 ABI candidates
and 414 layout-assertion candidates. These categories can overlap. These are
syntactic candidates, not confirmed bugs or an exhaustive data-flow audit;
layout assertions also protect valid on-disk/scalar formats.

| Boundary | Finding / next action |
| --- | --- |
| Renderer / texture registry | Native pointer storage; renderer prefix dispatch and fixed extent removed in this batch. Keep handles and dimensions fixed-width. |
| Graphics device / SDL GPU resources | Device pointers, shared ownership and numeric GPU texture IDs already use native/explicit types; Windows HWND/HRESULT boundary remains. |
| `legacy_memory.hpp` | Enforces 4-byte pointers and converts addresses through int32_t. Retain the guard until callers and object storage migrate; simply widening the helper would corrupt fixed records. |
| `legacy_abi.cpp`, `legacy_method_entries.cpp` | Win32 method ABI and x86 entry adapters still block the full game. Replace at typed caller boundaries. |
| ACT texture/font/layout records | `TextureResourcePrefix` includes a native vtable pointer plus original offsets. `FontRenderer` and `FontAtlas` now use native storage; their former 404/436-byte layouts are historical evidence. Outer ACT string/glyph and resource-prefix records remain to migrate. |
| Squirrel host wrappers / registrations | `squirrel_host_object.hpp` assumes 8-byte HSQOBJECT and 12-byte wrapper storage and exposes integer addresses. Registration and diagnostic callers also narrow pointers. Audit with the pinned VM/bytecode ABI; do not blindly redefine all VM integers. |
| Diagnostics | Some pointer casts are only logged or discarded, while others drive branches. Classify separately; logging width alone is not the game ABI migration. |
| Full-game build gate | CMake still requires MSVC Win32. The x64 result here covers the runtime type/listener contract only, not a complete game or GPU execution. |

## Next scope

Separate remaining ACT resource ownership from original byte-layout records, then
migrate remaining typed host/VM boundaries and x86 entry adapters. Keep file and
save formats explicitly fixed-width. Full x64 game compilation and user runtime
validation are separate milestones. Retired COM fixture assertions remain a
known coverage gap recorded in `../legacy-render-contracts/README.md`.

Build evidence is recorded in BUILD.md. No game or local test was executed.

## Font runtime checkpoint

`font_runtime.hpp` replaces byte-backed font/atlas storage with native pointers,
an embedded list and a native string. Rasterization/upload accepts typed renderer
pointers; `string_font_configure.cpp` isolates the remaining x86 ACT bridge.
CP932, GDI raster/outline operations, texture handles and explicit prune releases
are preserved. DAT/save formats are unchanged. Original shallow pixel-pointer
copy and borrowed atlas relocation semantics are retained; this is not a general
shared-buffer owner. See `font-original-layouts/README.md`.

The font/atlas contract is part of portable builds. On Windows the actual font
raster/upload translation unit also compiles independently as an object target.
GDI remains Windows-specific; this does not deliver cross-platform fonts or an
x64 game. Runtime assertions and visual equivalence have not been executed.

## ACT and host width checkpoint

The render path now reads ACT texture values through `act_texture_bridge.hpp`
into `TextureRuntimeState`, a native-width render snapshot. The duplicate
`TextureResourcePrefix` was removed. This snapshot does not own a texture; the
factory now constructs a native `TextureResource`; its explicit method table
and shared publication prefix retain the x86 adapter boundary.
The texture handle remains a 32-bit registry index.

Glyph queue allocation and clone ranges derive from `StringGlyphRecord` fields
instead of literal 256/20/24/232 byte ranges. The outer string and glyph records
still have their original 260/256-byte x86 layout. Their property schema uses
field offsets; changing the objects requires migrating serialization and all
virtual/quad callers together.

The shared 24-byte string boundary can now place its two owner pointers in the
16-byte inline area at either Windows pointer width. SqPlus object storage can
be compiled with native pointer width; SqPlus and Sqrat payload-reference
returns no longer narrow through `int32_t`. The old integer-address APIs and
class-binding/actor records still require x86. `legacy_memory.hpp`,
`legacy_abi.cpp`, `legacy_method_entries.cpp` and the top-level full-game CMake
gate remain explicit blockers. The x64 checks compile selected objects only;
they are not a linked game or executed contracts. See BUILD.md for artifacts.

ACT texture/target factories, clones, archive and script properties, mesh
replacement, draw access and cleanup now use a native `TextureResource`.
Clone references live in the object rather than a global registry. Native
members replace the 100-byte layout and fixed texture offsets. Chip resources
now also use native storage/shared MCD ownership, with a single representation
for map and script consumers. Both resource kinds use native allocation metadata. The portable contract compiles actual string ownership and
resource methods, not just a snapshot. See [scope and compatibility boundaries](act-texture-native.md).

## Chip resources and ACT interfaces

[Chip/method migration scope](act-chip-methods.md) records the full ownership
and call-chain migration. Method calls now have named, typed interfaces across
resource loading/querying, document I/O, layer/key cloning, script binding,
layout rendering and suspend/resume. An explicit method-table adapter remains;
its existence must not be confused with a portable full-game ABI. The native
contracts compile resource lifetime, allocation and callback arguments at x64.

## Native mesh resources and model decoder

Mesh resources now use native controller state and render ownership, replacing
the 248-byte overlay and fixed +236 linked-list consumer. The 3D layout calls an
actual C++ render interface. Texture replacement, clones, script properties and
release paths follow the native resource type. MSH/MAT decoding uses a portable
byte-source callback; the existing archive service is a separate adapter. Actual
resource lifetime and decoder code compile in independent Windows x64 contracts.
See [scope and retained behavior](mesh-native.md). GPU execution, the mesh-manager
ABI and complete game portability are not established by these compile checks.
