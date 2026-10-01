# 0.75x TAS pacing

Source `4bebdac` accepts 75-percent pacing alongside 25/50/100/200/400 and
advertises pacing-075-v1. It uses the existing SDL clock scheduler; simulation
still advances at a fixed logical 60 Hz. F8 request support is included.

Windows x64 Release delivery:
`runtime-builds/modern-x64-tas-speed075-01/kinoko_modern_gpu.exe`.
Build and DAT copy/hash verification passed. Logs, manifest and source revision
are retained in `build-runs/modern-x64-tas-speed075-01`.
Fonts/shaders and original three DAT are beside the executable.
No game was run by the agent.

Pair with editor `windows-editor-40-aot` (source `aeaafbe`), whose complete native
headless regression suite verifies the 0.75x selector and safe capability handling
alongside the redesigned edit/save workflow. This is simulated-engine evidence,
not measured real-game pacing or gameplay validation.
