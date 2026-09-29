# Windows recording replayed on WSL (2026-09-29)

User explicitly authorized WSL execution of the recording in the Windows
`modern-windows-replay-paths-01` package. No game source was changed.

## Inputs and isolation

- Both binaries were built from `b6c525f096b41fc2b7d7f7798e586cf7f72fed2d`.
- Windows session: `session-20260929-214155-491-a15d7db2` under
  `runtime-builds/modern-windows-replay-paths-01/6kinoko-modern-windows-x64-b6c525f0/recordings/`.
- Recording: 4005 frames, 833137 bytes; initial save/config snapshot is empty.
- Linux artifact: run `36576318325`, artifact `11037147397` (`game-ubuntu-24.04`).
- Windows runtime/snapshot hashes, Linux package manifest hashes and copied DAT
  and recording hashes were verified. Original recording and saves are unchanged.
- Linux binary/assets are retained under
  `/home/kinoko/games/wsl-replay-01/6kinoko-modern-linux-x64-b6c525f0/`.
  Three attempts use separate empty save directories under `wsl-replay-01`,
  `wsl-replay-02` and `wsl-replay-03` in `/home/kinoko/games/`.

This is a deliberate cross-platform diagnostic: the Linux CLI received the
original session identity explicitly. The ordinary launcher would reject the
different executable/shader hashes. No replay bytes, RNG state or checkpoints
were modified to make the run pass. This does not establish platform-neutral
session compatibility or remove the launcher's identity checks.

## Result

All three attempts detected the same first mismatch at frame **3444** (zero-based,
approximately 57.4 seconds of simulation). Frames 0 through 3443 passed the
existing checks. The recording did **not** complete successfully on Linux.

```text
FAILED at frame 3444: Desync at frame 3444;
checkpoint expected 46932037776b36a8, actual 8976d95f4f33a45c;
RNG expected c22d0c04, actual c22d0c04
```

The RNG states match; the selected game-state checksum differs. The aggregate
checksum does not identify which actor/global/camera field differs. Floating
point behavior is only a possible explanation, not an established cause.
Next investigation should compare detailed Windows/Linux state at this frame.

## Window visibility

First Wayland and then X11 attempts had invisible windows, reported by the user.
WSLg Weston logged `rdp_allocate_shared_memory: Failed to open ... Input/output
error`. Logs were retained. After game processes ended, WSL was shut down and
restarted (Windows was not rebooted). The user confirmed the third Wayland run
was visible; the fresh Weston log had no matching shared-memory error. It still
reported the identical frame-3444 desync, separating visibility from replay state.

First two runners terminated the game after detecting failure while the error
dialog was pending. The final visible run exited with process code 0 but status
`FAILED 3444 frames`; the replay status, not process exit code, is authoritative.

## Retained evidence

Windows: `build-runs/wsl-replay-01/` contains input audit, preparation and launch
scripts, initial logs, and `wsl-replay-02/` / `wsl-replay-03/` result records.
Linux: `/home/kinoko/games/wsl-replay-01` through `wsl-replay-03` retain runtime,
recording, save directories, invocation arguments, process and status logs.
Downloaded archives, slow partial downloads and verified download chunks remain
in `runtime-builds/wsl-replay-01/`. Stalled download processes were stopped after
the complete ranged archive was verified. No old artifacts were deleted.
