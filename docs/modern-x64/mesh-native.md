# Native mesh resource and decoder migration

Mesh resources now own native ResourceState and Render objects. The 248-byte
resource overlay, 140-byte reserved controller area, sentinel RenderLink list,
duplicated render count, private renderer byte table and 3D consumer's +236
list lookup are removed. Resource allocation uses the same aligned native
metadata as texture/chip resources; resource clone failure, archive-property
failure, Dispose and deleting-destructor paths all use the matching release.

Controller children/materials/models/lookup views retain their ownership
relationships. Renderers borrow models and replacement texture handles.
Renderer collection/draw order remains depth-first insertion order; failed bind
still keeps the renderer and a failed draw does not skip later renderers. Reload
releases renders before old models; destruction releases models/controllers
before renders. Native render destruction explicitly visits front to back.

The resource schema and Squirrel properties use actual native member offsets.
The generic x86 ID/name/method-table prefix is retained and asserted. Clone names
have independent string owners. Load still returns false even after successful
loading, empty-name loads leave state intact, null prefixes reuse the stored
prefix, and texture replacements keep their original multimap/borrow semantics.
The existing authorized controller-model bug fix and final debug triangle remain.

MSH/MAT decoding now reads a portable Input callback in mesh_model.cpp;
mesh_archive.cpp adapts the existing Windows archive reader. Serialized bytes,
version gates, unknown-node handling, index count-dependent width and material
color conversion are unchanged. The game uses this same decoder implementation.

Portable compile-only contracts cover actual resource ownership, render visit
and teardown order, reload/name independence, supported model/material versions,
16/32-bit indices, invalid versions and truncation. They compile production
resource/model/string code. Runtime assertions and gameplay have not been run.
The GPU-specific renderer and complete game still require Windows x86. The old
mesh-manager child/update ABI, outer ACT layouts, VM bindings and Windows services
remain separate migration work; this batch does not claim a full x64 game.
