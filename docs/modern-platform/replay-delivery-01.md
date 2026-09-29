# Startup replay delivery (2026-09-29)

Source: `5f071959edf74dc68e65bcd638730b378ca28cad`, branch
`codex/sdl-platform` in `Poker-sang/6kinoko-modern`.

This batch unifies logical frame publication and adds isolated startup recording,
playback, a deterministic session clock, RNG observations and selected game-state
checkpoints. It preserves ordinary launch and existing input configuration.
See [usage, format and limitations](replay.md).

## Windows delivery

Local package directory:
`C:\WorkSpace\6kinoko-modern\runtime-builds\modern-windows-replay-03\6kinoko-modern-windows-x64-5f071959`

- `launch.cmd`: ordinary game.
- `Record-Replay.cmd`: create an initial snapshot and record in an isolated copy;
  closing the game finalizes the recording.
- `Play-Replay.cmd`: replay the latest session in another fresh copy.
- `REPLAY.md`: instructions and current limitations.

The original three DAT files were copied beside both the raw and packaged EXE;
sizes and SHA256 hashes matched the originals. Package-manifest hashes were also
verified. The distributable ZIP excludes proprietary DAT as usual.

Build/log directory: `build-runs/modern-x64-replay-03/`.
Raw runtime: `runtime-builds/modern-x64-replay-03/`.
Earlier builds, logs and synthetic session fixtures remain retained.

## Verification

Windows x64 Release game and requested contracts compiled successfully.
Five local Windows checks passed: `input_actions`, `input_script`, `replay`,
`replay_runtime`, and `replay_session` contracts. Logs are `replay-tests.log`
and `launcher-tests.log` in the build directory. No actual game was launched.

The runtime contract uses synthetic frames, the real actor index and Squirrel
RNG. It verifies replay ignores live input and stops at injected actor/RNG
divergence. The launcher check uses fake files without executing an EXE and
verifies initial-save restoration, normal-save isolation and tamper rejection.
These results do not establish original-game playback determinism.

Initial CI revealed a POSIX `index` naming collision, a missing Win32 test log
stub and a Windows PowerShell hash-cmdlet dependency. All three were corrected;
file hashing now uses streaming .NET SHA256. Replay errors are reported on the
main thread after the worker joins, as required by SDL message boxes.

CI: https://github.com/Poker-sang/6kinoko-modern/actions/runs/36573637802
All seven jobs succeeded: full Windows x86/x64 builds and focused Windows
contracts, full Linux/macOS compilation, and all three portable compile jobs.
Linux/macOS game tests were not executed.

User gameplay validation is pending. This version does not implement mid-game
recording, frame editing, fast-forward or save-state restoration. Checkpoints
cover selected state rather than the entire VM.
