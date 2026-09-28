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

Three-word SqPlus actor/camera arguments, class-binding storage, integer-address
registration, direct method calls outside these adapters and embedded ACT/actor
layouts remain migration work. Their guards must not simply be removed. Native
aggregate support in the common dispatcher does not migrate those VM records.
GDI/Windows services follow the full x64 milestone; save/DAT scalar formats stay
fixed-width. Prior user feedback: modern-string-native-01 normal (user report).
