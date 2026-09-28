# Full-game x64 milestone

The complete Windows x64 game now compiles and links: `modern-x64-integer-01`,
source `7f1044f0614b5b40ff5c03643760d23aa3de4968`. It remains experimental:
no game or tests were executed, and real DAT/save compatibility is unvalidated.
See [startup crash fix and delivery](startup-integer-fix.md) and
[build evidence and historical checkpoints](BUILD.md).

## Build the real game graph

Commit source first and use a fresh name; retain all products and logs:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_x64_probe.ps1 -Name <unique-name> -SourceDir C:/WorkSpace/6kinoko -Generator "Visual Studio 17 2022"
python tools/summarize_x64_build.py --build build-runs/<unique-name>
```

Despite its historical name, this script builds the complete game target with
`-A x64`, records compiler/linker results and stages three DAT on success.
It does not run the game or build all contracts. The bytecode wire contract was
compiled separately in the final batch. Win32 remains available through
`tools/build_staged.ps1`, which also builds active contracts.
Keep `shaders` and three DAT beside the EXE. No reference-working-directory override.

## Compatibility boundaries

Native pointers/VM objects are separate from fixed-width disk scalar formats.
ACT and script dispatch preserve explicit receiver/reserved arguments and typed
floats. Original serialized hashes and CV4 fields retain 32-bit semantics.
Legacy-memory guards remain; original x86 layout assertions apply on x86.
Compilation does not prove all runtime pointer flows, savedata or real bytecode.

Next: user x64 runtime acceptance, then remaining Windows service migration for
full Linux and macOS builds. Historical scope: [ACT/map](act-map-publication-native.md),
[input/audio/application](input-audio-application-native.md), [bindings](binding-act-native.md).
