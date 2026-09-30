# Small CPU fast-seek optimization

Final runtime: `runtime-builds/modern-x64-tas-cpu-02/kinoko_modern_gpu.exe`.
Built commit `17ad1cf` (runtime change `32de7d2`). Editor unchanged.

Only while fast_seeking(), ordinary screen-space quads use the renderer's
existing inline four-vertex representation. Expansion into triangles occurs
when actually submitted, including cancellation/target frames and offscreen
passes. Other primitives and normal gameplay/replay keep their old path.
Only while advancing in fast seek, control mailbox reads are limited to every
2 ms. Normal speed, stationary boundaries and the target poll immediately.
Commands still execute on a simulation boundary; cancellation can arrive a few
fast frames later. Game updates, draw callbacks, RNG and replay checks remain.

Before restricting quad expansion to fast seek, three paired real-replay runs
in cpu-01 measured mean 7.773 seconds on pipeline-02 versus 7.041 seconds on
the optimized build for 15,535 simulated frames: 10.39% higher throughput,
9.41% less time, approximately 1,999 to 2,206 FPS. All six output recordings
were SHA256-identical to the source and all six final RGBA previews matched.
This small gain is retained because implementation scope is small.

The final fast-only build was separately compiled and checked: TAS edit and
runtime replay contracts, synthetic GPU target/cancel/bounded-queue/offscreen
checks passed. Its real recording verification completed in 6.757 seconds,
with identical recording bytes and identical final preview to pipeline-02.
That single-run time is validation, not a replacement for the three-run mean.
No new manual normal gameplay validation is claimed. Normal operation retains
the prior code path through explicit fast-mode gates.

All logs, inputs/hashes, generated recordings, build trees and fixtures remain
in `build-runs/modern-x64-tas-cpu-01` and `build-runs/modern-x64-tas-cpu-02`.
Three DAT files were staged and SHA256 checked next to the final executable.
Select the new executable and reopen the editor session to use it.

User priority: preserve ordinary gameplay/replay, avoid large changes for small
gains, and investigate substantial measured bottlenecks before further changes.
Full VM snapshots remain deferred. No further optimization is included here.
