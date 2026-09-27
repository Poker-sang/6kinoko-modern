# Native chip resources and ACT method interfaces

This batch completes the chip-resource and ACT call-interface stage following
native textures. It does not claim a full-game x64 port.

- One native ChipResource replaces the full 100-byte chip record and the map
  consumer's duplicate 68-byte prefix. Factory, loader, clone, property schema,
  script properties and map caches all use its real fields.
- Decoded MCD has std::shared_ptr ownership with the existing kinoko_mcd_free
  deleter. Loading publishes only after success; reload replaces the receiver's
  ownership without invalidating clones. Each resource keeps independent names.
- Texture/chip allocation has aligned native size_t metadata. Scalar and array
  cleanup uses constructed C++ objects and reverse array destruction, without
  the VC8 four-byte cookie. Existing deleting-destructor flags remain adapters.
  Mesh and other ACT objects retain their separate allocators.
- Named ResourceMethods, DocumentMethods, LayerMethods, LayoutMethods and
  SerializableMethods replace distributed resource/ACT slot arithmetic. The
  compatibility header deliberately retains real virtual callbacks and the
  Win32 thiscall boundary; it re-reads method tables for each operation.
- Resource query order, result accumulation and mutation-sensitive range reads
  remain unchanged. The render-target resource pass now reads native dimensions
  rather than the obsolete +72/+76 record offsets left after texture migration.
- Document reading/cloning, ownership, archive writing/type lookup, resource
  class binding, layout update/draw and layer association/position use the named
  interfaces. Mesh-manager and sprite interfaces are separate non-ACT services.

New portable contracts compile production native resource allocation/string
code, shared MCD lifetime, long-name copying, reload independence, array order,
invalid/overflow counts and typed method adapters including table replacement
and floating-point draw arguments. Existing resource-pass and map-cache fixtures
now model native resource storage. Contracts are compiled, not executed.

Remaining platform scope: outer ACT/string/glyph fixed layouts, full VM binding
pointer width, mesh/controller storage, non-resource allocation conventions and
Windows services. These still block a full x64/non-Windows game; the interface
adapter is an explicit migration boundary, not a claim that its old ABI vanished.
