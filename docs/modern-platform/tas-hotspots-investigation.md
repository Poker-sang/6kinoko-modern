# Fast seek hotspot investigation (2026-10-01)

Decision: no production optimization retained. Keep using
runtime-builds/modern-x64-tas-cpu-02/kinoko_modern_gpu.exe.
All runtime and test sources at closeout are identical to a4e9560e.
Experimental commits and all artifacts remain available for further analysis.

## Workload and coarse results

Same unpacked 15,536-frame recording from
6kinoko-tas/artifacts/windows-editor-27/sessions/session-20261001-005151-f4e74c,
with independent saves and 15,535 simulated frames per seek. Windows x64 Release.

Opt-in instrumentation in hotspots-03 measured approximately:

| Scope | Milliseconds |
| --- | ---: |
| Wall time | 6941 |
| Full update (inclusive) | 4872 |
| Draw callbacks and packet preparation | 1922 |
| Actor update | 2260 |
| Global script callback | 690 |
| Checkpoint | 480 |
| Stage update | 273 |
| Command boundary IO | 177 |
| Render queue wait | 143 |
| Map update | 127 |
| Camera update | 84 |
| Replay writer | 38 |
| Physical input polling | 1.5 |

Scopes overlap; do not sum this table. The scene timing includes game updates,
checkpoint and replay work. Main-thread render processing overlaps the worker.
This recording used no transformed vertices, ruling out matrix-composition
caching as an improvement for this workload.

## Rejected capacity reservation

Prototype 5625415 reserved screen draw/texture vectors from observed demand,
bounded at 4096 entries and enabled only during fast seek. Commands, references,
draw order and callbacks were unchanged. It added only 11 implementation lines,
but showed no benefit.

Same-instrumentation baseline d2d434ce versus prototype, three runs each:

| Variant | Milliseconds | Mean |
| --- | --- | ---: |
| Baseline | 7537.7963, 7256.3867, 7394.8916 | 7396.3582 |
| Reserve | 7530.7543, 7534.9941, 7471.0522 | 7512.2669 |

Prototype mean was 1.57% slower. Removed in 3b5f796.
TAS edit and replay runtime contracts passed for the prototype.
Every output recording matched the input SHA256; all six final images matched:
1255DB50CE3D19550322570438A1326DEDB25BC5F6269A131CDBD918B061A6EE.

## Function-level investigation

Windows WPR CPU capture failed to enable the system performance policy
(0xc5585011). No trace session was started. A separate Release build with
symbols and temporary native-call timers provided a fallback. No system policy
was changed. Native reports were separated by thread after the first attempt
overwrote the game report with the loader report.

native-profile-02 (877cad0), game thread, approximate inclusive native timings:

| Entry | Calls | Milliseconds |
| --- | ---: | ---: |
| LoadAnimationData / LoadAct / LoadMap shared wrapper | 9 | 652 |
| SqPlus instance getter | 3,417,455 | 542 |
| Squirrel closure.call | 128,703 | 534 |
| Map chip lookup by position | 15,031 | 181 |
| SqPlus instance setter | 702,407 | 84 |

The shared loader wrapper cannot distinguish the three resource operations.
Native timing includes startup and nested calls, and timer overhead; it is for
hotspot identification, not a speed benchmark or additive time breakdown.
Do not mistake these counts for redundant reads that can simply be removed.
Instance metadata and script callbacks are observable mutable state.
The archive reader reopens resources but this alone does not establish that a
cache would provide a worthwhile benefit or preserve resource override behavior.

The evidence points to script/property work and resource transitions as future
investigation areas, rather than GPU submission, transformed vertex math or
recording writes. No property metadata cache, resource cache, changed collision
algorithm, VM snapshot, skipped callback or reduced checkpoint coverage is shipped.

## Retained evidence and final state

- build-runs/modern-x64-tas-hotspots-01 through -03: coarse timing builds.
- build-runs/modern-x64-tas-reserve-01: benchmark inputs, timings, hashes and tests.
- build-runs/modern-x64-tas-sampling-01: symbol build and WPR rejection log.
- build-runs/modern-x64-tas-native-profile-01 and -02: native timing builds,
  linker maps, raw reports and resolved.txt (-02).
- Corresponding runtime-builds directories contain EXEs and SHA256-staged DATs.

All successful real replay runs completed byte-identically; final image hashes
were checked across the coarse and native-timing builds as well. No manual
gameplay validation is claimed. Temporary timers and the rejected reservation
were removed completely; this batch delivers investigation records, not a new
game executable. No final rebuild is necessary for unchanged runtime sources.
