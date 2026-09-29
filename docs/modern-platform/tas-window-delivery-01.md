# TAS direct window delivery

Source 4ac9775, codex/tas-bridge; no Mod merge in this batch.
Game: runtime-builds/modern-x64-tas-window-02/kinoko_modern_gpu.exe.
Editor: C:/WorkSpace/6kinoko-tas/artifacts/windows-editor-06/KinokoTAS.App.exe, source 04324e8.

--tas-window selects the normal SDL GPU swapchain instead of hidden offscreen
readback. It requires --tas-dir and the existing isolated replay/save options.
Default bridge mode stays embedded for compatibility. Native window keys match
the editor defaults; F9 toggles run/pause, F10 requests one frame. Keyboard state
is sampled on the SDL main thread and exchanged atomically with the worker.
Losing game focus clears its mask; external editor focus changes do not pause.
Switching to unrelated apps does not auto-pause either: pause explicitly first.
No platform-specific window reparenting or foreign process hooks.

Editor remembers engine path and display mode, opens recording then connects,
uses Chinese statuses and clearer record/continue/save labels. Input drafts are
explicitly separate from live recording. Old recordings/saves remain untouched.

Validation: targeted game/replay/runtime/preview builds succeeded. replay_contract
and replay_runtime_contract passed (tests.log), synthetic hidden GPU readback
passed (preview.log). Three DAT staged and hash-verified (dat.log). Editor
build-16/checks-16 passed, including external launch argument and focus transfer.
No actual gameplay or performance benchmark was run. The embedded transport is
unchanged; only direct window mode eliminates the readback/file image path.

The all-target x64 build also attempted historical Win32-only contracts and failed
in legacy_memory.hpp/address adapters; build.log retains this failure. It is not
reported as a complete all-target success. Earlier partial build-01 and all logs
are retained. Relevant delivery targets succeeded in target-build.log.
