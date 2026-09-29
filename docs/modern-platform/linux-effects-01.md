# Native star / floating-score investigation

The user reproduced a block-spawned star and player death in WSL using source
92dfbd07, but did not reproduce a stomp score in this recording. The original
trace is retained in `/home/kinoko/games/wsl-effects-01/6kinoko-modern-linux-x64-92dfbd07/kinoko-trace.log`.
The bounded extract is `build-runs/linux-effects-01/star-observed.txt`.

## Confirmed star defect

Frame 125631: take 1060, UpdateWalk, position (529,768), velocity
(-67748.4,9), visible/active, valid texture 520. By frame 125640 x=-609206;
the actor eventually releases below the map. This is invalid script motion,
not a missing GPU texture.

Original item.cv4 InitStarD divides rand() by literal 32767.0 before computing
angle and speed (speed = random fraction * 3 + 3). Original executable source
at 0x4c6cae registers RAND_MAX=0x7fff; the native math adapter calls CRT rand.
Linux/macOS libc rand use a different range/sequence. The portable replacement
now reproduces the Windows CRT 32-bit state transition, 15-bit output, default
seed and per-thread shared state on every OS. No shader or actor physics change.
Keep this vendor math-library patch when updating Squirrel.

InitStarA and InitNumber do not themselves call rand. Do not infer that this
fix resolves every star variant or floating-score disappearance. Score remains
unconfirmed/unfixed pending an actual reproduction trace. Diagnostics now cover
all take-1060 update callbacks and score takes 1104..1109. Native opt-in
`KINOKO_TRACE=1 KINOKO_TRACE_VERBOSE=1 KINOKO_TRACE_FILTER=star` limits output to
effect/error records; default gameplay still has no trace file.

The new random contract checks known CRT outputs, full-range seeds, original
star-speed bounds and (when compiled with MSVC) comparison with the real CRT.
Build/delivery and actual execution results will be recorded below.

## Delivery

Source: `067fc414dabc51e9aea7e54958aeb592dbbaf988`.
[CI 36553498171](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36553498171)
passed all six jobs, including Linux and universal macOS full games. Local
Windows x64 game and random contract compiled successfully at
`runtime-builds/modern-x64-effects-01/kinoko_modern_gpu.exe`; three DAT hashes
verified. Windows game/contract were not executed.

Linux random contract from the portable CI artifact was executed in WSL and
passed (exit 0); log: `build-runs/linux-effects-01/random-contract.log`.
Native package manifests and all three staged DAT hashes verified. Executable
modes explicitly verified in final tar archives (Windows-mode intermediate
archives retained under separate names). Local-only data-containing packages:

- `runtime-builds/modern-linux-effects-01/6kinoko-modern-linux-x64-067fc414-with-data.tar.gz`
  SHA256 `47f202851729b515afcd40b84d778ce450e542094e49cf67e4bed41989134d30`.
- `runtime-builds/modern-macos-effects-01/6kinoko-modern-macos-universal-067fc414-with-data.tar.gz`
  SHA256 `53f06686f926afbe987b10f36ade05397dc3544693d38f06de15dab77462fe15`.

Fresh WSL directory:
`/home/kinoko/games/wsl-effects-02/6kinoko-modern-linux-x64-067fc414`.
Launcher: `runtime-builds/modern-linux-effects-01/launch-wsl-diagnostics.cmd`.
The WSLg COPY MODE/invisible-window problem recurred; Weston evidence retained
and WSL restarted. Agent observed visible startup graphics afterward. This is
startup evidence only; user gameplay verification and floating-score trace
remain pending. Original full trace retained; new run uses the star filter.
