# Complete native-width game build

Source commit: `e720bc466dda17c600023c84a9cd3680da79ae3d`.

## Delivered artifacts

- `modern-full-x64-24`: complete Windows x64 Release no-trace game compiled and linked, zero compiler/linker errors. Static PE inspection confirms AMD64 / PE32+.
- EXE: `runtime-builds/modern-full-x64-24/kinoko_modern_gpu.exe`.
- SHA256: `43915aa22f5276a0f3cba295f4570689f1c25da25c8956b05f804b4573061088`.
- Three original DAT files copied beside the EXE and size/SHA256 verified; both sprite DXBC shaders staged under `shaders`.
- `kinoko_bytecode_wire_contract` additionally compiled and linked as AMD64; not executed.
- `modern-full-width-03`: complete Win32 Release no-trace game and 78 contracts compiled, DAT verified. EXE: `runtime-builds/modern-full-width-03/kinoko_modern_gpu.exe`.
- D3D9 static audits passed for x64 (25 compiler / 4 linker logs) and Win32 (122 compiler / 88 linker logs), zero violations.
- No game, CTest or contract executable was run. Experimental x64 gameplay, real DAT bytecode loading and save compatibility remain unvalidated. Compilation is not proof that all pointer flows or runtime behavior are correct.

## Implementation

File readers, bitmap/sprite relationships, actor methods, render queue and host dispatch use native fields and explicit receiver/reserved arguments. Actual pointer consumers no longer depend on truncated diagnostic addresses. Original x86 layout evidence remains conditional; the legacy-memory pointer-width guard remains intact.

VM objects, CSV/script arguments, child VM creation, refcounts and savedata streams preserve full pointers. Integer outputs cross into game int32 values explicitly. CSV/load/save registration uses typed aggregate thunks, avoiding an accidental x86 flattened-argument ABI. Type identities and print callbacks retain their actual pointer/function types.

Table diagnostics use SQTable iteration and a read-only source-VM refcount observer instead of fixed node sizes and offsets. Actor diagnostics use native object pointers. Trace calls have not been compiled out.

The live VM retains native SQInteger/pointer storage. Closure serialization explicitly reads/writes original 32-bit little-endian tags, counts, integer literals and metadata; savedata scalar writes remain int32. A new independently constructed synthetic CV4 fixture covers read/write byte identity, integer/float literals, execution result and truncated input rejection when later executed. It is not extracted original-game evidence and has only been compiled. Vendor changes are documented in `third_party/squirrel-2.2.2/README.kinoko.md`.

Legacy Windows API wrappers are restricted to x86; x64 links real system imports. The build summarizer now records linker failures as well as compiler diagnostics.

## Evidence and retained attempts

See `full-x64-24-artifacts.json`, `full-x64-24-blockers.json`, `full-x64-24-delivery.json`, `full-x64-24-d3d9-audit.json`, `full-width-03-artifacts.json` and `full-width-03-d3d9-audit.json`.

All intermediate build/run trees and logs remain: full-x64-18 through 22 exposed remaining compile/ABI/link issues; full-x64-23 first linked successfully; full-width-01/02 exposed old C fixture adapter dependencies. Final x64-24 and width-03 use the same source commit. No older artifacts were overwritten or deleted.

## Next acceptance boundary

User gameplay verification of the x64 deliverable comes next, including real resource/bytecode loading and eventual save compatibility. Linux/macOS complete-game builds still require remaining Windows services (including fonts) to migrate. Existing isolated platform-module builds do not establish complete-game portability.
