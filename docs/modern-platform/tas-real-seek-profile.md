# Real replay fast seek profile

Windows background validation authorized by the user on 2026-10-01.
The game ran with its hidden embedded window and real SDL GPU backend. The
editor was not running these replay processes; its existing sessions, saves,
recordings and executable selection were not changed. This measures steady
seek rather than restart/resource loading or editor UI overhead.

## Evidence

Baseline executable: `runtime-builds/modern-x64-tas-fast-seek-04/kinoko_modern_gpu.exe`
(source `80271671`). Instrumented executable:
`runtime-builds/modern-x64-tas-profile-01/kinoko_modern_gpu.exe` (source `188d125a`).
The retained runner is `tools/profile_tas_seek.ps1` (commit `2068d47d`).
It copies initial files into fresh run directories and never writes to the
source recording or initial save directory. Input SHA256 values, generated
recordings, status files, final previews and timing results are retained under
`build-runs/modern-x64-tas-profile-01/{baseline-01,profile-01}`.

The existing real recording has 15,536 frames. Each process starts paused after
frame one, then seeks through 15,535 additional frames. Three baseline runs
measured 13.652, 14.234 and 13.509 seconds (mean 13.798 seconds). Three profiled
runs measured 13.667, 14.230 and 13.555 seconds (mean 13.817 seconds).
Throughput is approximately 1,100-1,150 FPS. The means differ by 0.14%, below the
observed scheduling variability. These are sequential runs, not simultaneous
benchmarks. All six finalized output recordings were SHA256-identical to the
input and reported completing all frames without replay desynchronization.

## Breakdown

Mean worker update time: 6.986 seconds, approximately 51%.
Mean worker draw callback/CPU packet construction time: 2.481 seconds, approximately 18%.
Mean worker render-queue wait time: 4.327 seconds, approximately 31%.
Mean main-thread packet processing time: 0.103 seconds, less than 1% of wall time.
Boundary mailbox/state handling takes approximately 1.217 seconds and is already
included in update time; main-thread packet processing overlaps worker waiting.
These overlapping values must not be added together as independent percentages.
The update category also includes replay verification/writing and normal update
services, so it is not purely Squirrel VM execution.

Each run deferred 15,534 screen-only packets and submitted the final target
packet. There were no mixed or offscreen-only packets in this particular replay.
Thus the conservative mixed-pass handling is not the bottleneck here. The
profile omits the first simulated seek frame in worker stage totals because the
seek command activates profiling inside that update; final target/pause handling
is included in wall time. This has negligible effect at this replay length.

## Next optimization

Further GPU shader or presentation optimization cannot significantly improve
this case: intermediate GPU output is already suppressed. CPU packet generation
and per-frame main-thread synchronization remain substantial. The next useful
experiment is a bounded render queue during fast seek, preserving ordered passes
and exact target/cancel acknowledgement, followed by reducing disposable screen
packet construction without omitting draw callbacks that can mutate game state.
Even eliminating all current render waiting would cap the isolated improvement
at roughly 1.45x; eliminating both drawing and waiting would approach 2x, which
is a theoretical bound rather than a promised implementation result.
Complete VM serialization is not needed to investigate these improvements.

Profiling is opt-in through `KINOKO_TAS_PROFILE=1` and writes aggregate
`seek-profile.txt` records in the independent TAS bridge directory. Normal
recordings do not produce these measurements. No optimization or replay wire
format change is delivered in this profiling batch. Release compilation,
TAS edit/runtime replay contracts and the synthetic GPU target/cancel/offscreen
contract passed. Build logs, DAT SHA256 staging and all test fixtures are retained.
