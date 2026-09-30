# Bounded fast seek render pipeline

Windows runtime: `runtime-builds/modern-x64-tas-pipeline-02/kinoko_modern_gpu.exe`.
Built source: `e81134e`. Select this executable in the editor and reopen the game
session; an existing session keeps its original executable. The editor is unchanged.

During intermediate fast seek frames, the game worker can continue with fewer
than four outstanding render packets. The main thread processes up to four
packets per poll in FIFO order. BeginScene uses the same fast queue capacity as
worker backpressure. Target frames drain the queue fully, and cancellation waits
for the exact most recently simulated frame preview before acknowledgement.
Ordinary gameplay/playback retains its previous rendering and queue behavior.
Draw callbacks, offscreen dependencies and mixed pass order are preserved.
The fixed one/two-triangle immediate primitive allocates its final vertex storage
once before construction, instead of repeatedly growing its temporary vector.
No full VM state snapshot, image history cache or replay format change is added.

## Real recording comparison

Background Windows runs use the same real 15,536-frame recording and initial
state as the prior profiling batch. Each hidden embedded run starts paused after
frame one and measures seeking the remaining 15,535 frames. Startup/resource
loading and editor overhead are excluded. Measurements were run sequentially.

| Run | Baseline seconds | Optimized seconds |
| --- | ---: | ---: |
| 1 | 14.401 | 7.740 |
| 2 | 14.687 | 7.786 |
| 3 | 14.470 | 7.825 |
| Mean | 14.519 | 7.784 |

Mean throughput rises from approximately 1,070 to 1,996 FPS: **1.865x speed**,
or **46.39% less seek time** for this recording. The baseline is the prior opt-in
profile executable (`188d125a`), whose overhead was previously measured below
the observed run variability; both executables use the same profiling mode.
The benchmark runner revision is `95d39bdb`.

Worker render-queue waiting falls from a mean 4.671 seconds to 0.094 seconds.
Measured CPU draw construction falls from 2.490 to 2.154 seconds; measured update
time falls from 7.336 to 5.515 seconds. Those update times include boundary IO and
replay verification. These observations do not isolate the allocation change from
scheduling effects. Main-thread packet processing remains approximately 0.1 seconds.
Every run defers 15,534 intermediate screen-only packets and submits one target
packet; this replay has no intermediate mixed/offscreen packets. Other recordings
and machines can therefore show different gains. The earlier isolated wait-time
bound assumed the other stages stayed constant; here their measured costs also fall.

All six runs reported completing all frames without desynchronization. Their
finalized output recordings are SHA256-identical to the source recording. All
six final RGBA previews also share the same SHA256:
`1255DB50CE3D19550322570438A1326DEDB25BC5F6269A131CDBD918B061A6EE`.

## Validation and retained artifacts

Release build, TAS edit and runtime replay contracts passed. The synthetic GPU
contract covers overlapped intermediate frames, blocking at the queue limit,
complete draining at the target, cancellation with queued frames, exact target
and cancel pixels, and offscreen texture retention across intermediate frames.
The three DAT files are staged beside the executable and SHA256 verified.

Logs, source commit, comparison JSON, image hashes and fixtures are retained in
`build-runs/modern-x64-tas-pipeline-02`. Real replay runs are under `optimized-01`
and `baseline-02`. Original recording, initial files, editor settings and active
user sessions were not overwritten. The initial failed queue-capacity contract in
pipeline-01 and baseline-01 runner mailbox failure are retained. The latter was
resolved by retrying atomic command replacement while a Windows reader holds
the mailbox; it was not a replay desynchronization.
