# Native script bindings and ACT ownership

## Completed scope

The common SqPlus class-registration record and method payload use native
pointers. Variable metadata mirrors the source VarRef with a native-width
address/offset/constant slot and real declaring/value descriptor pointers.
Instance resolution writes a pointer; static/root properties preserve their
whole storage address. Strings, userdata and typetags use pointer outputs.
Scalar game properties stay 32-bit, with explicit conversion from the VM's
native SQInteger before memory writes.

The SqPlus source constructor, ancestry publisher and declaring-base lookup now
share internal type-key operations. x86 preserves the original integer keys and
raw-set publication. x64 uses userpointer keys, avoiding collisions between
addresses with equal low words. Source reference ownership, metadata publication
order and stack restoration remain intact. Vendor changes and checksums are
recorded under third_party/sqplus-20080713/PATCHES.*.

Single-VM closure adapters use native return types matching SQFUNCTION. The
common method dispatch, global registrations and actor creation/update adapters
compile against those signatures. Captured native function addresses and VM
thread-owner storage use native sizes. Compile-time signature checks cover both
widths; ordinary game functions keep their existing 32-bit scalar results.

ACT document, layer, script, callback, key/timeline and runtime ownership records
use native pointer layout. Script payload IO and text copying share the canonical
script fields; callback and query prefixes are tied to those fields with layout
assertions. Document/layer creation, cloning, destruction, parent/resource
association, layer queries, frame updates and runtime lifetime compile at x64.
Fixed ACT scalar fields, timeline disk pairs and serialized hash IDs remain
32-bit. Historical x86 layout assertions remain active.

Typed VM object assignments, destructors and source GC adapters use the supplied
Squirrel implementation directly. Only their historical x86 evidence assertions
are conditional; the old integer-address helper remains guarded for unmigrated
callers. Host and script objects keep external references, not SQObjectPtr
ownership. Acquire/release order and selective initialization remain unchanged.

## Evidence and limits

The isolated width target compiles 30 actual production translation units, including the
source-backed SqPlus implementation, plus one layout/signature assertion file.
PE/COFF inspection confirms AMD64 objects. The full Win32 build compiles all
active contracts, but neither contracts nor the game are executed by the agent.
See BUILD.md for source commits, fresh directory names and artifact manifests.

The complete x64 game still has remaining dependencies: ACT's monolithic
publisher has legacy local object slots; 2D/3D/map/draw layouts, map/collision,
input/application records, some script thread/generator/array bridges and host
services still require migration. Standalone source compilation does not prove
that those downstream consumers are portable.

Before calling an x64 build playable, also audit VM bytecode and save-number
serialization against the original DAT format: upstream _SQ64 changes SQInteger
width. Native property conversion does not establish compiled-script format
compatibility. No existing file format was widened in this batch.

User reports modern-actor-native-03 normal. That is user feedback, not agent
runtime validation of this batch.
