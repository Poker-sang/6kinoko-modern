# Fast checkpoint lookup experiment (2026-10-01)

Decision: rejected; production runtime remains identical to 87a95fad.
The fast-only prototype replaced root-table enumeration with nine raw table
lookups through SQTable::Get. Normal replay retained enumeration. Public
sq_rawget was unsuitable because a missing key changes the VM last error.

Windows Release prototype bb667a06, runtime/build directory
modern-x64-tas-checkpoint-02. Same 15,536-frame recording as prior measurements;
15,535 simulated frames per seek, three sequential runs for each executable:

| Variant | Times (ms) | Mean (ms) |
| --- | --- | --- |
| Existing cpu-02 | 6945.4124, 6891.9873, 7167.3289 | 7001.5762 |
| Prototype | 6839.6089, 6752.6223, 6923.6353 | 6838.6222 |

Time reduction 2.33%, throughput increase 2.38%. Ranges overlap; this is not
strong evidence of a worthwhile gain. Do not recommend this prototype as an
upgrade. No VM snapshot work or gameplay changes were introduced.

All six output recordings matched input SHA256. All six target RGBA hashes:
1255DB50CE3D19550322570438A1326DEDB25BC5F6269A131CDBD918B061A6EE.
TAS edit, replay runtime and synthetic GPU target/cancel tests passed for the
prototype. No manual normal gameplay validation is claimed.

Keep enhanced replay fixtures: fast seek against normal recording, tracked
scalar/null/table values, absent root keys versus delegated keys, preservation
of VM stack and last error. The first fixture attempt used an unregistered
base-library helper; checkpoint-01 failed setup and is retained. checkpoint-02
uses the public VM API to install the delegate and passes.

All builds, DAT staging evidence, recordings, timings, profiles and image hashes
are retained under build-runs/modern-x64-tas-checkpoint-01 and -02.
Continue using runtime-builds/modern-x64-tas-cpu-02/kinoko_modern_gpu.exe.
The next investigation should measure update/checkpoint/VM work separately
before choosing another optimization; update_ms includes checkpoint work, so
it is not evidence that script execution alone accounts for that time.
