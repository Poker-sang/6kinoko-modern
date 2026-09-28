# x64 savedata load/save fix (2026-09-28)

User reports: old saves crash during startup, and saving in the new game also
crashes. This batch targets both paths; runtime confirmation remains pending.

## Cause and fix

Both load_file and save_file retain the input Squirrel table through
Object::assign -> kinoko_squirrel_object_from_pair. Object::assign already uses
intptr_t, but the shared helper declaration/definition still accepted int32_t.
This discarded the high pointer bits before sq_pushobject and reference capture.
Use intptr_t at this native object boundary. On-disk payloads remain 32-bit.

Read signed integer keys/values as int32_t and sign-extend into the native VM.
Read floats as unsigned raw 32-bit IEEE bits so their high native bytes remain
zero. This prevents a negative old integer becoming a positive 64-bit integer,
and avoids corrupting float equality for values whose sign bit is set.

Serialize and compress fully before CREATE_ALWAYS, preserving an existing save
on buffer/conversion/compression failure. This is not atomic replacement and does
not claim protection against disk-write failures or interruption during writing.

## Evidence and regression coverage

Read-only Python struct/zlib inspection of original marisaA.dat and marisaB.dat
consumed the complete 14,105-byte decoded stream for each file. Their nested
containers/tags conform to the original format. Hashes and counts are retained in
savedata-original-wire.json. Neither original file was changed or staged into a
new runtime directory. This is format inspection, not game load verification.

The existing file contract covers actual source VM references, nested table/array
roundtrips and independent zlib wire fixtures. Added signature assertion ensures
native-width object payloads; golden fixture now covers a negative key, INT32_MIN
and negative float. Oversized serialization must preserve an existing save.
The fixture retains generated files for subsequent user-run investigation.

## Delivery

Source `150308d9833a6dcf12be512c39b100980e4ae13f`:
- x64 game: `runtime-builds/modern-x64-savedata-01/kinoko_modern_gpu.exe`.
  SHA256 b2daa7be4230f50f36143dcb57ef23c23b1f181aa83587fddb7a4e0fb7f860a8.
- Win32 game: `runtime-builds/modern-savedata-01/kinoko_modern_gpu.exe`.
- Full game builds succeeded on both architectures. Win32 compiled all 80
  contracts; x64 savedata file contract also compiled separately, not executed.
- Three DAT files staged with size/SHA256 verification; two sprite shaders present.
- Both static D3D9 audits passed. Optional diagnostic launcher is beside x64 EXE.

No game, CTest or contract executable run by the agent. All previous artifacts
retained. Save/load runtime acceptance is pending; prior gameplay-01 acceptance
covers the earlier four gameplay reports, not this newly reported savedata defect.
