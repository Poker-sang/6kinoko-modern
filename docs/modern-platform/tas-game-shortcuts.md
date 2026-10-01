# Portable game-window F8 requests

Source `a0c8db8` adds rising-edge SDL F8 detection for an external TAS window.
The engine publishes numbered `KTASKEY1 N toggle-recording` mailbox requests;
it does not take over recording itself. The editor owns sealing, recovery copies,
bookmarks and mode switching. Capabilities now include `shortcuts-v1`.
F9/F10 local pause/step behavior is unchanged.

Windows x64 Release delivery:
`runtime-builds/modern-x64-tas-shortcuts-01/kinoko_modern_gpu.exe`.
Build, source revision, staging hashes and manifest are retained in
`build-runs/modern-x64-tas-shortcuts-01`. Build succeeded and the original three
DAT files were copied and verified next to the executable; fonts/shaders retained.
Game was not run by the agent.

Pair with editor `windows-editor-39-aot` in `C:\WorkSpace\6kinoko-tas`.
Editor source `11b38f5`, native regression suite `artifacts/checks-aot-85` passed
with synthetic game requests and actual editor mode-switching/save logic.
This does not constitute real-game keyboard validation.
