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

Built source: `19becedcb9c160941ccb3ba8525b48e9c0b0f35a`.
[CI 36561889023](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36561889023)
passed all seven jobs: four full-game builds plus three portable builds. All
six focused contracts passed on each full-game job (24 passes total). macOS
binaries contain both architectures; execution is on the CI runner's host
architecture, not proof of executing both slices. Windows static D3D9 audits
also passed.

First CI 36561176403 exposed an old save-fixture syntax issue before file I/O:
Squirrel 2.2 rejects consecutive same-line table definitions separated that
way. Use supported newline boundaries in the fixture, without changing the
compiler or save implementation. Independent WSL compiler probes are retained
under `build-runs/closeout-script-probe-01` and `-02`. Final real save tests
passed on all four configurations.

GPU transfer executed in WSL CPU Vulkan/llvmpipe, exit 0, all pixels matched.
Log: `build-runs/closeout-gpu-01/transfer-result.log`. GPU artifact source was
15c50147; GPU test/backend source is unchanged in the final build. Metal and
D3D12 GPU contracts were compiled but not executed. No new gameplay run is
claimed. WSL compiler installation/probe artifacts and all old runs are retained.

Final artifact manifests and all three original DAT sizes/SHA256 were verified.
Native tar executable/launcher modes were checked and explicitly preserved.
Final packages (local only, original DAT never uploaded):


- `runtime-builds/modern-windows-baseline-01/6kinoko-modern-windows-x64-19becedc-with-data.zip`
  SHA256 `86744ffd6ca14867467f526bd3c3bce5d4986ae25e279c23eeb5794750d1e7ce`.

- `runtime-builds/modern-linux-baseline-01/6kinoko-modern-linux-x64-19becedc-with-data.tar.gz`
  SHA256 `741c20ad0e55748114759a75c576a34cdb3e2e54100c86fec6b459a8182604f7`.

- `runtime-builds/modern-macos-baseline-01/6kinoko-modern-macos-universal-19becedc-with-data.tar.gz`
  SHA256 `cdb6327ff9bc547c7d113ab0b7e2f8bd503c3c8116207aa8de83ff7f198fd781`.

Launch the extracted Windows `launch.cmd`, Linux `launch.sh`, or macOS
`Launch.command`; each package includes its separate diagnostic launcher.
Windows EXE is inside `runtime-builds/modern-windows-baseline-01/6kinoko-modern-windows-x64-19becedc/`.
WSL is deployed to `/home/kinoko/games/baseline-01/6kinoko-modern-linux-x64-19becedc`.
Local shortcuts: `runtime-builds/modern-linux-baseline-01/launch-wsl.cmd` and
`diagnose-wsl.cmd`. These do not shut down WSL or replace older packages/saves.
The new game was staged, not launched for gameplay.

Downloaded CI logs, tests/fixtures, packages and staging hash records remain in
`build-runs/closeout-final-{windows,linux,macos}` and
`build-runs/closeout-delivery`. Follow-up documentation commits do not change
the binary source revision above.
