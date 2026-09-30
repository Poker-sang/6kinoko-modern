# Live recording export and focus

snapshot-v1 handles `snapshot 0` at a paused replay frame boundary. The writer
flushes its output, copies the prefix and exports a matching footer to
tas-dir/recording.krec without finalizing or modifying the active stream.
The acknowledgement is published after the copy is ready. Recording can append
more frames and export again. This is a completed replay prefix, not a save-state.

focus-v1 handles `focus 0` by asking the main SDL thread to call SDL_RaiseWindow.
The Windows editor grants the game's process foreground permission before the
command. Embedded sessions use editor focus and do not require this command.

Delivery: `runtime-builds/modern-x64-tas-live-save-01/kinoko_modern_gpu.exe`,
source `7263d4df`; logs in `build-runs/modern-x64-tas-live-save-01`.
Replay, replay-runtime and TAS-edit contracts passed. Writer export preserves
appendability; real runtime/Squirrel tests save a paused prefix before resuming
the same writer. Three DAT are staged and hash-verified. Editor counterpart:
`C:/WorkSpace/6kinoko-tas/artifacts/windows-editor-24/KinokoTAS.App.exe`, source
`5d62e21`. No actual gameplay or physical window focus validation is claimed.
