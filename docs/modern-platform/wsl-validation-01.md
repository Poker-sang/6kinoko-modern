# Local WSL validation setup (2026-09-29)

User accepted macOS render-02 and authorized configuring WSL2 and running the
Linux game locally. No game execution has occurred yet in this task.

## Installed / checked

- Windows 11 Pro 10.0.26200.9550, AMD Ryzen 7 9700X.
- Firmware virtualization and second-level address translation both report true.
- WSL was initially absent. Installed through an elevated PowerShell helper,
  using normal Windows administrator authorization.
- WSL 2.7.14.0, Linux kernel 6.18.33.2-2, WSLg 1.0.73.2; default WSL version 2.
- Virtual Machine Platform enablement succeeded. Windows explicitly requires
  a reboot before the change takes effect. No reboot was initiated.
- `wsl --install -d Ubuntu-24.04 --no-launch` returned the reboot requirement;
  no Linux distribution is registered yet. Do not treat exit zero as a completed
  Ubuntu install or claim Vulkan/game runtime compatibility.

Installation transcript, exit records and decoded WSL status are retained in
`build-runs/wsl-validation-01/`.

## Prepared game package

Source `92dfbd07f007f69170b7bb440e6f9760433a3f65`, successful Linux CI artifact
11014915132 from run 36525803562. Downloaded package files matched the CI
manifest; the three original DAT were copied and hash-verified locally.

`runtime-builds/modern-linux-wsl-01/6kinoko-modern-linux-x64-92dfbd07-with-data.tar.gz`

SHA256: `81cbbae89f19baa57705be4a3384064960da615d745c7e194309f62c51832373`.
Download/staging evidence: `build-runs/linux-wsl-delivery-01/`.
DAT-containing archives remain local only.

## Resume after user reboot

1. Check WSL status and install Ubuntu 24.04 if still absent. Use WSL2.
2. Configure required Linux packages, inspect WSLg DISPLAY/WAYLAND/PulseAudio
   environment and Vulkan devices. Report hardware versus software rendering
   accurately; do not assume WSLg implies usable Vulkan.
3. Extract the prepared archive into a fresh directory on the Linux filesystem,
   retaining executable permissions, shaders/fonts and DAT beside the game.
4. With the user's explicit execution authorization, run the GPU transfer
   contract and launch the game, capturing actual logs/results. If interactive
   validation is available, keep it to first level, jump, monster, then exit.
5. Diagnose and fix concrete failures; preserve evidence and commit/push each
   completed batch. Do not mark the Linux platform validated merely because
   WSL installed or the native package compiled.
