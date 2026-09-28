# Full-game x64 milestone

The first hard milestone is a complete Windows x64 game, then a playable Linux
build, then macOS validation. A portable contract is not completion of that goal.

## Build the real game graph

Commit source first, then use a fresh name (all products and logs are retained):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_x64_probe.ps1 -Name <unique-name> -SourceDir C:/WorkSpace/6kinoko -Generator "Visual Studio 17 2022"
python tools/summarize_x64_build.py --build build-runs/<unique-name>
```

This configures the actual game/dependency graph with -A x64, builds only the
full game target, and records success or failure. Source address/layout guards
are NOT disabled. A failing compiler is a blocked milestone, never a working x64
build. On successful linking the three DAT are staged; the script never runs the
game or tests. The normal supported build remains Win32.

## Method-call migration

Reconstructed callbacks have an explicit unused EDX parameter. It remains a real
argument in x64. method_entry.hpp now passes it explicitly at both widths. ACT
resource/document/layout/key/layer dispatch and generic native script wrappers
use this convention. The old real-C++-member ABI helper remains for historical
ABI contracts; it is no longer the production script call route.

Rectangle, move and blit draw calls now pass floats as floats, not int32 bit
patterns. Pointer/resource parameters stay pointers. Captured function addresses
require sizeof(void*) bytes before reading. Low-word diagnostic logging remains
explicitly isolated from dispatch.

A contract checks mixed float/integer/pointer register and stack arguments,
high pointer bits and native aggregate dispatch. ACT fixtures now match actual
reconstructed two-parameter receiver signatures on x64 as well as x86. The actual
squirrel_native_calls.cpp is compiled independently with x64, not just a fixture.
No contract or game execution is performed by the agent.

## Remaining boundaries

Actor/camera SqPlus arguments and embedded slots now use native-width owned
objects. Common class/property storage and global closure returns now use native widths.
Remaining game-specific publication slots, direct method calls outside the migrated
adapters and embedded map/input/application layouts remain migration work.
Their guards must not simply be removed. Remaining ACT publication and VM
thread/array/generator consumers need migration beyond the common bindings.
GDI/Windows services follow the full x64 milestone; save/DAT scalar formats stay
fixed-width. Prior user feedback: modern-string-native-01 normal (user report).

## Windows services and serialized hashes

The full compiler exposed Win32-only register names, fixed lock-size assertions
and a host-size_t Boost hash dependency. Diagnostics now emit Rip/Rsp/Rbp and
64-bit fault addresses on x64 while preserving the x86 output branch. Critical
sections use native field layout, as all live consumers already do.

Serialized ACT hashes now explicitly reproduce Boost 1.44's Win32 unsigned
32-bit seed arithmetic and signed-char values on every host. Merely casting a
64-bit boost::hash_range result would change type IDs, so that is not used.
Compile-time assertions preserve existing original-layout/timeline/string IDs,
embedded zero and high-bit character samples. Actual hash adapter, diagnostics
and critical-section sources compile at x64. This is not runtime validation.

## Compiler checkpoint and next dependency groups

`modern-full-x64-03` configures the real game graph but fails compilation with
fixed-layout/address guards intact. It produces no x64 game. Diagnostics and
hash source errors from the preceding attempt are gone; progressing farther in
the graph exposes more guarded records (268 unique messages, including cascades).
See `full-x64-03-blockers.json` and BUILD.md; these are not 268 separate tasks.

That baseline led to the shared reference/container and actor/camera migration
below. The latest probe is modern-full-x64-05 (438 unique diagnostics, including
cascades; see full-x64-05-blockers.json). Next migrate ACT/document/layer/script
storage and map/input/collision dependencies. Complete the remaining by-value VM
and direct callback boundaries before claiming a full-game x64 link. Keep build
probes tied to source commits and preserve each failing attempt for comparison.


## Native actor/camera and ownership checkpoint

Actor and camera script slots now match the native Squirrel external-reference
object (12 bytes on x86, 24 on Windows x64). Callback arguments transfer that
object by value through the explicit receiver/reserved-argument method adapter.
They retain the existing consume-once and callback destruction order. Actor copy
keeps retain-before-release, pool reuse does not zero unrelated actor state, and
animation selection keeps its existing upper-only index clamp.

Pool allocation uses sizeof its real host, animation frame addressing and
SetTake results preserve pointer width, and map/vector/reference/buffer storage
uses native pointers. Original scalar/file layouts and packed actor handles stay
fixed-width. All historical actor x86 field assertions remain enabled on x86.
The integer-address legacy-memory guard remains enabled for unmigrated callers.

Seventeen actual actor/camera/container/lifecycle translation units compile as
AMD64 in modern-actor-native-x64-02. The new ownership contract links actual
Boost control and integer containers, covering reference locking/expiration,
alias clearing, stable map nodes, vector growth and native script payload bits.
Its runtime assertions were compiled, NOT executed. This is not a linked x64 game.

Next work follows the full compiler: migrate Squirrel class registration and
property/object bridges together with ACT document/layer storage, then map,
collision, input and application records. Actor collision/method registration
and the complete VM are not covered by the isolated actor compile target.
User reports modern-x64-entry-03 normal; this is user feedback only.


## Common binding and ACT ownership checkpoint

See [binding/ACT scope](binding-act-native.md). Common SqPlus metadata and
pointer outputs, internal type identity keys, native closure return signatures,
ACT document/layer/script/callback/runtime records and their ownership consumers
now compile independently at native width. The remaining full compiler errors
are tracked in the latest BUILD.md checkpoint. The complete x64 game is still
blocked, and source VM bytecode/save widths require explicit original-format
verification before a playable DAT-compatible release.


Latest compiler checkpoint: modern-full-x64-12, build source f32f24c6108ee1d42be9477d1432f4457ba9a197.
133 unique diagnostics remain (including cascades), down from 438 in full-x64-05.
The 30-source isolated native target compiles successfully; no x64 game is linked.
See full-x64-12-blockers.json for the exact unresolved source locations.
