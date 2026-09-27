# x64 preparation: runtime graphics records

This batch prepares selected graphics runtime objects; the complete game remains
Windows x86. The user reports modern-cleanup-02 currently normal. That is user
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
| ACT texture/font/layout records | `TextureResourcePrefix` includes a native vtable pointer plus original offsets. `FontRendererRecord` retains its 404-byte layout and native GDI/owner pointers. Separate these runtime owners from byte-backed views next. |
| Squirrel host wrappers / registrations | `squirrel_host_object.hpp` assumes 8-byte HSQOBJECT and 12-byte wrapper storage and exposes integer addresses. Registration and diagnostic callers also narrow pointers. Audit with the pinned VM/bytecode ABI; do not blindly redefine all VM integers. |
| Diagnostics | Some pointer casts are only logged or discarded, while others drive branches. Classify separately; logging width alone is not the game ABI migration. |
| Full-game build gate | CMake still requires MSVC Win32. The x64 result here covers the runtime type/listener contract only, not a complete game or GPU execution. |

## Next scope

Separate font/ACT resource ownership from original byte-layout records, then
migrate remaining typed host/VM boundaries and x86 entry adapters. Keep file and
save formats explicitly fixed-width. Full x64 game compilation and user runtime
validation are separate milestones. Retired COM fixture assertions remain a
known coverage gap recorded in `../legacy-render-contracts/README.md`.

Build evidence is recorded in BUILD.md. No game or local test was executed.
