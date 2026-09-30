# Live recording export and focus

snapshot-v1 handles `snapshot 0` at a paused replay frame boundary. The writer
flushes its output, copies the prefix and exports a matching footer to
tas-dir/recording.krec without finalizing or modifying the active stream.
The acknowledgement is published after the copy is ready. Recording can append
more frames and export again. This is a completed replay prefix, not a save-state.

focus-v1 handles `focus 0` by asking the main SDL thread to call SDL_RaiseWindow.
The Windows editor grants the game's process foreground permission before the
command. Embedded sessions use editor focus and do not require this command.
