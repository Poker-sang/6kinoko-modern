# SDL platform foundation — modern-01

Current production work: [portable file services](file-services.md), after the
user-confirmed x64 gameplay and savedata fixes. The sections below describe the
historical first SDL platform batch; renderer, audio and native-width migration
have since progressed. They are not the current blocker inventory.

Fork baseline: rebuild `af5dd9a`. Remote: `Poker-sang/6kinoko-modern` (private).
The source checkout and existing uncommitted rebuild evidence remain untouched.

## Implemented boundary

SDL3 owns the game window, event pump, keyboard, mouse and joystick collection.
The portable `kinoko_platform` has no Windows or DirectX headers. A synchronized
frame cache crosses from the main SDL thread to the original game update thread.
SDL USB scancodes map explicitly to the saved legacy scan IDs. Axis conversion
preserves -1000..1000 and the original physical input threshold remains +/-500.
Disconnected joystick slots are neutral and retain their assignment index;
new devices append (16 slots maximum). Reconnected devices need reassignment.
SDL raw joystick ordering may differ from DirectInput; existing controller
assignments should be reviewed. Keyboard defaults preserve their identifiers.
Relative mouse motion is accumulated until consumed once by the game thread.
Focus loss clears input. SDL native HWND is borrowed only at the Windows bridge.
The legacy Alt+Enter D3D toggle remains in a thin WndProc adapter.

Pinned source: SDL 3.4.16, libogg 1.3.6, libvorbis 1.3.7. See dependencies.json
for source URLs and archive SHA256. Builds need no dependency downloads.
Upstream license files remain alongside each library. Historical codec sources
are retained as evidence but are no longer linked. Squirrel 2.2.2, SqPlus, zlib
and Boost are not upgraded in this batch: their ABI, script and serialization
contracts require separate migration, not an automatic version bump.

## Build

Portable modules (Windows x64, Linux, macOS):

```sh
cmake -S . -B build-runs/platform-01 -DKINOKO_PLATFORM_ONLY=ON -DCMAKE_BUILD_TYPE=Release -DKINOKO_RUNTIME_DIR="<absolute-runtime-directory>"
cmake --build build-runs/platform-01 --config Release --parallel 4
```

Full transitional game (Windows x86 only), after committing source:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_staged.ps1 -Name modern-01 -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

The three DAT files must sit beside the game EXE; build_staged copies and hashes
them. Game and contract execution remain with the user. A successful compile
is not a passing runtime test. CI builds portable contracts on three OSes and
the complete Windows bridge; it does not run games or tests.

## Evidence and limits

IDA MCP survey is preserved in survey_binary-survey.json and E-imports.json. The
NoAutoAnalysis database had no decoded function at 407500/408C80, so its disasm
outputs are NOT successful function verification. Automatic approval rejected
a supplementary IDA database decode operation with 'blocked by policy'.
Input behavior is instead grounded in existing reconstructed physical_input.cpp
(407500), direct_input.cpp (408C80), and src/decompiled/6kinoko.exe.c.
No new dynamic or gameplay validation was performed. Squirrel VM/bindings were
not modified; the existing vendored 2.2.2 implementation and prior evidence are
retained, not claimed as newly disassembled in this batch.

This is the first platform migration, NOT a cross-platform game release.
D3D9/D3DX rendering, DirectSound output, Win32 threading/IME/filesystem services,
and x86 pointer/layout assumptions remain. Next migrate renderer and audio,
then runtime services and pointer-width boundaries before enabling the complete
game on other OSes. No TAS or Mod behavior has been added yet. Split attack/run
and enter-door/climb are future logical-action/script work, not implemented by
physical key remapping.

Old rebuild automation workflows are archived under legacy-workflows/ so they
cannot mutate this fork or push old branches. Current CI is platform.yml.

The MCP import classifier labels SendMessageA as network; this is a category
heuristic error, not evidence of networking. Actual imports include Win32 file,
window, synchronization and multimedia services. Minimal survey alone omitted
imports; E-imports.json preserves the subsequent standard survey.
