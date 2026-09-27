# Window move presentation handoff

Source commit: `cbda7725748a29535c5ffab75d8cf24e20bec394`.

Win32 enters its own modal loop while the title bar is dragged or the window
is resized. The normal SDL message loop cannot call `kinoko_graphics_poll()`
during that interval, although the game and display workers continue to queue
frames. The window procedure now polls the GPU queue on its own 16 ms window
timer and on move/size messages. It keeps presentation on the original main
thread, forwards ordinary window/input messages to SDL, and removes the timer
when the modal loop ends or the application shuts down.

- `modern-window-move-01`: full Win32 Release game and 69 contract executables
  compiled. The three original DAT were staged beside the EXE and verified by
  size and SHA256. Keep the adjacent shaders directory.
- EXE: `runtime-builds/modern-window-move-01/kinoko_modern_gpu.exe`.
- EXE SHA256: `82ED524871EDF315D08467721998986421E370ED44F1705C17217DD89F1B5E8E`.
- Static D3D9 audit passed: 105 compiler and 79 linker dependency logs, no
  violations. Logs and manifests remain in `build-runs/modern-window-move-01`.
- No game or contract executable was run. Window drag, held drag, resize,
  minimize and restore behavior require user runtime verification.
