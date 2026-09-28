# Native string layout and glyph ownership

This batch replaces the original 260-byte CStringLayout and 256-byte glyph
records with native C++ storage. Factory, ACT deserialization, script properties,
layer aliases, clone, update/draw and destruction now use the same representation.
Glyph geometry uses QuadState without a fake sprite vtable. Other legacy quads
retain their ABI and share only the submission implementation.

Atlas pages have stable addresses and shared ownership. Growing or erasing the
page vector cannot invalidate a glyph. ReplicateText retains pages until the last
layout/glyph owner releases them; each page releases its texture exactly once.
Glyph copy/destruction maintains the original reference count used by pruning.
Queue replication copies before replacement to preserve the destination on failure.
Native allocation metadata supports throwing constructors with reverse rollback.

Compatibility deliberately retained:
- CP932 default font and Windows CharNextA/CharPrevA byte traversal.
- Original tab/wrap, alignment, vertical centering and color calculations.
- Pending characters are consumed before the visibility check.
- PopFront's pending ASCII quirk (zero bytes erased).
- Rebuild truncates at embedded NUL, while archive text concatenation preserves it.
- Script queueCount reads pending byte length, not glyph count.
- Archive alignment still aliases the edge boolean; script alignment is separate.
- Clone copies independent text/style/cursor state and leaves both caches empty.

The string storage/lifetime contract compiles actual native code and shared string
ownership. It covers page address stability, source destruction after replication,
clone cache semantics, embedded NULs, texture release and allocation rollback.
It is compiled only; no tests or game are run by the agent.

The game remains Windows x86: GDI rasterization, character traversal, layer records,
script integer-address bridges and the ACT method-table ABI remain separate work.
User reports modern-mesh-native-01 normal; this is user feedback, not agent testing.
