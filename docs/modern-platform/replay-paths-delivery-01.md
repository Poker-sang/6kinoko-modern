# Shared-executable replay delivery (2026-09-29)

Source: `b6c525f096b41fc2b7d7f7798e586cf7f72fed2d` on
`Poker-sang/6kinoko-modern`, branch `codex/sdl-platform`.
The user reported that the preceding startup replay version worked well;
that is user feedback, not agent gameplay validation.

## Behavior

The Windows session launcher now starts the same packaged EXE for normal play,
recording and playback. It snapshots only writable DAT/config files. EXE,
archive DAT, fonts and shaders remain in the game directory and are hashed in
place. Each playback reads the original recording directly, with a fresh copy
of initial saves/configuration. It does not duplicate the executable, assets or
recording file. No hard links, symlinks or special filesystem permissions are
required. The frame codec remains unchanged and uncompressed.

`--save-dir` redirects table saves, legacy key configuration and logical action
configuration. There is no fallback to ordinary saves when the selected folder
has no save. `--record` / `--replay`, `--replay-status` and `--replay-identity`
select session files explicitly; the launcher writes exact arguments to
`launch-arguments.json`. SDL handles Windows Unicode command-line decoding.
Resource loading still uses the executable directory regardless of launch cwd.

The launcher validates EXE/resources and initial saves/configuration before
playback. Direct manual invocation must preserve those files itself. Older
version-1 full-copy sessions remain usable with their retained original package;
the new launcher uses manifest version 2 and does not migrate or delete old files.
See [replay guide](replay.md) for commands and limitations.

## Delivery and verification

Windows package directory:
`C:\WorkSpace\6kinoko-modern\runtime-builds\modern-windows-replay-paths-01\6kinoko-modern-windows-x64-b6c525f0`

Use `Record-Replay.cmd`, `Play-Replay.cmd`, or ordinary `launch.cmd`.
Raw runtime: `runtime-builds/modern-x64-replay-paths-01/`.
Build and logs: `build-runs/modern-x64-replay-paths-01/`.
Three original DAT files were copied beside both delivered/raw EXEs and checked
for size and SHA256. Package manifest file hashes were verified. ZIP excludes DAT.

Windows Release compilation succeeded. Seven local checks passed:
`runtime_services`, `input_actions`, `input_script`, `savedata_file`, `replay`,
`replay_runtime`, and `replay_session`. Logs: `contracts.log` and
`launcher-tests.log`. Tests cover actual Unicode selected-directory save I/O,
path-escape rejection, explicit-path playback independent of cwd, initial-save
restoration, no asset/replay duplication, and changed-EXE/snapshot rejection.
All fixtures and previous builds remain retained. No game gameplay was run.

CI: https://github.com/Poker-sang/6kinoko-modern/actions/runs/36576318325
All seven jobs succeeded, including Windows x86/x64 compilation and focused
contracts, plus Linux/macOS compilation. No Linux/macOS gameplay or contracts
were executed in this batch. New-version gameplay validation remains user-owned.
