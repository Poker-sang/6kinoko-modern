# Native ACT texture resources

Texture and render-target resources now use `TextureResource`, constructed with
native member defaults. Their allocation is independent of the chip resource's
100-byte original layout. Native texture objects contain no original unknown or
padding fields, and accidental shallow copy/assignment is disabled.

Migrated together: factory/defaults, clone/deep string copy, retained texture
ownership, explicit unload/load, destruction, array element stride, property
serialization, Squirrel property registration, draw snapshots and target creation.
The texture clone lease map/mutex is removed: the retained handle lives in the
object and is consumed once independently of the visible handle. Owned and borrowed
textures share one explicit unload transition. Render-target destruction still
restores the backbuffer before releasing an owned target.

Compatibility boundaries:
- Method-table, ID and name prefix remains compatible with x86 generic ACT users;
  compile-time assertions protect this boundary. Method tables and VM adapters
  are not yet a portable full-game ABI.
- String fields retain the existing native-backed StringRecord boundary. Clone
  names have separate owners; archive property names/types/order are unchanged.
- Texture/chip allocation now uses aligned native metadata for scalar and array
  ownership. The deleting-destructor adapter retains flag compatibility, but
  reads a native count rather than the old four-byte array cookie. Cleanup and
  C++ destruction run in reverse order before freeing the allocation.
- Empty-name loads still preserve the old handle; suffix order remains DDS/BMP/PNG.
  Target creation still returns 1 on failure and preserves original replacement
  semantics, including separate retained clone references.
- Chip/mesh resources, outer ACT/VM ABI and full-game x64 migration remain pending.

The portable texture contract now compiles the actual production resource,
render bridge and legacy_string.cpp. Cases cover long-name deep copy, independent
string lifetime, constructor defaults, crop copying, snapshot fields, mutable
visible handle versus retained reference, borrowed cleanup and repeated unload.
Runtime assertions are compiled only, not executed.

The missing-length `kinoko_string_assign_cstr` entry now resides beside its
Windows scanner in legacy_string_scan.cpp. The explicit-length native string
implementation links independently for portable contracts; full-game string
behavior is unchanged. Mesh texture replacement and diagnostic texture access
also use the native object, eliminating their old +68/+40 reads.
