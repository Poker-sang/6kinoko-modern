# Cross-platform replay math correction (2026-09-29)

Source: `e8c53ffd93190a4c693c5aef6afbfb3e08a749db` on
`Poker-sang/6kinoko-modern`, branch `codex/sdl-platform`.

## Cause and change

The user's original 4005-frame Windows recording first diverged on Linux at
frame 3444. Detailed state comparison identified exactly one different field:
actor pool handle `50266478`, actor ID `366`, horizontal velocity:

| Value | Windows | Prior Linux |
| --- | --- | --- |
| velocity_x bits | `409ca7a6` | `409ca7a7` |
| velocity_x decimal | 4.895464897155762 | 4.89546537399292 |

Script math called `cos()` with SQFloat input bits `3f5710c4`. Microsoft's C
math header exposes the double entry point; POSIX's C++ math header also exposes
float overloads. The Linux call selected float precision. Under the original
game thread's upward rounding, it returned `3f2ad9fd`, whereas double evaluation
followed by SQFloat narrowing returns `3f2ad9fc`. The differing result propagated
into velocity. RNG states were equal. The mismatch was not a different random
seed or input sequence.

All single/two-argument Squirrel math wrappers now explicitly promote operands
to double and narrow the returned result to SQFloat, preserving the original
Windows evaluation path independently of header overloads. No random values,
stored replay bytes, checksums or comparison tolerances were changed.

The real-VM replay contract tests this exact cos operand/result under FE_UPWARD.
Opt-in checkpoint diagnostics use `KINOKO_REPLAY_DETAILS=<UTF-8 output path>` and
`KINOKO_REPLAY_DETAILS_FROM=<first frame>`; they record raw integer/float bits for
actors, selected globals and aggregate state. They remain disabled by default.
The temporary per-math-call instrumentation was removed from the final source.

## Validation and limitations

- The initial field-diagnostic Windows build replayed all 4005 original frames.
- After correction, detailed Linux state matched the Windows reference in all
  **71369 rows** across frames **3438 through 4004**, including the formerly
  divergent actor. Earlier frames had no replay-checkpoint failure.
- Final Windows build: five focused input/replay/launcher contracts passed.
  Final WSL build: the real-VM replay/math contract passed.
- Final WSL replay with ordinary trace enabled exited normally with
  `COMPLETED 4005 frames` (66.02 seconds). This was actual automated gameplay
  playback, authorized by the user for this investigation.
- A second final replay with ordinary trace and field diagnostics both disabled
  also exited normally: `COMPLETED 4005 frames` (66.03 seconds).
- The first corrected Linux run with detailed field logging reached frame 4004
  without a mismatch but timed out during exit. Its status was not COMPLETED;
  its logs and timeout result remain retained. A subsequent run exited normally.
  This isolated timeout is not claimed fixed by the math change.
- Corrected Windows playback was stopped early at frame 3914; the user explicitly
  confirmed manually closing it. A later attempt stopped at frame 999 during a
  turn interruption. Neither is reported as complete or as a desync.

CI https://github.com/Poker-sang/6kinoko-modern/actions/runs/36583828135 succeeded
in all seven jobs, including Windows tests and Linux/macOS compilation. This
does not prove every recording/platform math function is bit-identical. Normal
launcher EXE/shader identity checks remain; cross-platform diagnostics supplied
the original identity explicitly and preserved the original recording.

## Delivery and retained evidence

Windows package:
`C:\WorkSpace\6kinoko-modern\runtime-builds\modern-windows-replay-math-02\6kinoko-modern-windows-x64-e8c53ffd`

Original DAT files were staged beside the raw and packaged EXE and hash checked.
Linux DAT hashes and the Windows package manifest were also verified.
Normal launcher: `launch.cmd`; recording/replay launchers are also included.

Windows build/logs: `build-runs/modern-x64-replay-math-02/`.
Windows raw runtime and playback attempts: `runtime-builds/modern-x64-replay-math-02/`.
Linux source/build: `/home/kinoko/build-runs/replay-math-02/`.
Linux runtime and playback attempts: `/home/kinoko/games/replay-math-02/`.
Earlier `replay-desync-01/02` and failed `replay-math-01` builds are retained.
The latter failed from a vendor newline/macro editing mistake, corrected before
the final successful builds. Original recordings and all older packages remain intact.
