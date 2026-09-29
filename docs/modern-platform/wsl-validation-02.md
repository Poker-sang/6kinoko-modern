# WSL Linux startup evidence (2026-09-29)

User authorized an official image download/import and Linux game execution.
The user also reports missing spawned stars and stomp-score effects on both
Linux and macOS source `92dfbd07`; same-source Windows x64 render-02 is normal.
This task sets up a reproducible Linux runtime; it does not resolve that issue.

## Environment

The stalled `wsl --install` child PID 8744 held an established TCP connection
to 127.0.0.1:12345, whose listener was xray PID 24608. Its partial image was
preserved before stopping only the two installation processes. No proxy
configuration was changed.

Downloaded official image from:
`https://cloud-images.ubuntu.com/wsl/releases/24.04/current/ubuntu-noble-wsl-amd64-24.04lts.rootfs.tar.gz`

356736729 bytes; SHA256 matched the official SHA256SUMS:
`2a790896740b14d637dbdc583cce1ba081ac53b9e9cdb46dc09a2f73abbd9934`.
Download and partial old image retained in
`C:/WorkSpace/wsl-images/ubuntu-24.04-20260929/`.

Imported WSL2 distro: `Kinoko-Ubuntu-24.04`.
VHD storage: `C:/WorkSpace/wsl-distributions/Kinoko-Ubuntu-24.04/`.
Created local non-root account `kinoko`, with no login password set. Installed
Ubuntu Vulkan/audio runtime packages. Mesa 25.2.8 reports Vulkan 1.4.318 on
**llvmpipe CPU software renderer**. Do not label it GPU acceleration.

## Actual execution

Game source: `92dfbd07f007f69170b7bb440e6f9760433a3f65`.
Runtime (Linux filesystem):
`/home/kinoko/games/wsl-validation-02/6kinoko-modern-linux-x64-92dfbd07`.
The previously hash-verified archive was extracted intact, with original DAT,
fonts/shaders and executable permissions. No data-dir override is used.

The GPU transfer contract ran and reported a mismatch in its final
`64x7x1 padded=1 zero_rows=1` case. Earlier cases reached completion before this
first mismatch. The whole contract **failed**, not passed. This mixed-zero
descriptor default case is distinct from normal game uploads, which set both
dimensions. Do not attribute the gameplay issue to it without more evidence.

First game launch had audio but an invisible WSLg window. Weston reported
shared-memory open failure (Input/output error) and the window title included
`[WARN:COPY MODE]`. Switching X11 to Wayland alone did not fix it. After retaining
logs and shutting down/restarting the sole WSL distro, the warning disappeared
and the agent observed a correctly visible title/menu screen through Computer
Use. The game was left open for the user; no first-level/gameplay/save pass is
claimed. No game source changes were required for this WSLg session failure.

## Launch again

Windows shortcut script (local):
`runtime-builds/modern-linux-wsl-01/launch-wsl.cmd`

Equivalent command:
```powershell
wsl.exe -d Kinoko-Ubuntu-24.04 -u kinoko --cd /home/kinoko/games/wsl-validation-02/6kinoko-modern-linux-x64-92dfbd07 -- env SDL_VIDEO_DRIVER=wayland XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir ./kinoko_modern_gpu
```

Logs: `build-runs/wsl-validation-02/`, including image download/hash, package
installation, Vulkan summary, GPU contract failure and Weston before/after
restart. Retain all artifacts. Next investigate the cross-platform missing
effects separately and distinguish CPU Vulkan evidence from native Linux GPU
and Apple Silicon behavior.
