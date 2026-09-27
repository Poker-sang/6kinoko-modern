# Original font layouts

Historical x86 evidence: renderer 404 bytes, atlas 436 bytes. These headers are
not compiled or installed. Active runtime objects use `font_runtime.hpp` and
native pointer alignment. Outer ACT string/glyph records remain fixed-layout.
The current single-character path leaves the pixel list empty and bitmap null;
shallow buffer-pointer copies retain the original limitation and are not a safe
general-purpose shared-buffer ownership model. Texture release remains explicit
in atlas pruning, and borrowed glyph atlas pointers retain relocation semantics.
