# Cross-platform closeout

## Accepted behavior

User confirmed all three symptoms (block stars, balloon stars, stomp scores)
resolved in the Linux/WSL build 067fc414. Earlier Apple Silicon texture fixes
were also user-accepted. This does not claim a fresh macOS test of 067fc414.

## Compatibility audit

- Script RNG is explicitly MS CRT 32-bit state/15-bit output, independent of
  libc. Bytecode contract additionally executes actual registered math functions,
  negative integers/floats and effect-style postincrement counters.
- DAT/CV4/save wire fields remain fixed-width, separate from native pointers.
  Move actual save/bytecode/archive tests before the Windows-only CMake return.
  Standard C++ creates retained fixtures; no original saves are touched.
- Archive lookup uses shared slash normalization and CP932-aware ASCII case
  handling. Added mixed-case/backslash resource lookup to the real archive test.
  Host filenames remain host-sensitive: package names and DAT filenames use
  canonical case. No recursive case-insensitive filesystem fallback introduced.
- Clock values use unsigned 32-bit elapsed differences and SDL delay; Windows
  retains its documented uptime epoch/resolution adapter. Existing runtime
  service contract covers clock and synchronization. This is a targeted audit,
  not proof that all reconstructed gameplay code is platform-independent.
- D3D12 and Vulkan independently default zero row/height transfer fields.
  Correct the earlier test's combined-default assumption and Metal upload AND
  download. Exercise all combinations, padded rows/slices and both directions.
  Game uploads specify both fields, preserving accepted rendering.

## Delivery workflow

CI builds Windows x86/x64, Linux x64, universal macOS from one revision. Six
focused non-gameplay contracts run on each full-game job: savedata_file,
bytecode_wire, file_archive, script_random, runtime_services, directory_search.
Test logs/retained save and archive fixtures are included in CI artifacts.
GPU transfer remains manual (WSL software Vulkan can execute it locally).

All packages include source/hash manifests, shaders/fonts/licenses, normal and
separate diagnostic launchers. Ordinary launch explicitly disables traces even
if the parent shell enables them. Diagnostics default to low-volume records.
Original DAT are never uploaded. Native tar permissions are set explicitly,
including when the local packaging host is Windows.

User handoff: on each target verify entering/clearing a level, star/score effects,
save then quit/restart/load. Automated fixture roundtrips are not this gameplay
check. Next feature phase is input action separation/shared input layer, after
user acceptance of this baseline; TAS and MOD are not implemented in this batch.

## Validation and packages

Pending the committed build; final evidence is appended after execution.
