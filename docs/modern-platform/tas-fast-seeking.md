# Unlimited TAS positioning

`seek-fast-v1` adds a `seek` mailbox command with a completed-frame target.
It removes artificial frame delays until the target, then pauses and restores
ordinary pacing without changing the selected playback speed. Fixed simulation
clock, input, RNG, checkpoint verification and per-frame drawing callbacks are
unchanged. Backward positioning still requires a new process and replay.

TAS drawing drains its queued render work outside graphics locks before the
next frame boundary. This prevents the bounded queue from dropping the final
frame. Embedded sessions render normally but download only the target image;
cancellation requests a download of the actual current framebuffer before
pause acknowledgement. External sessions temporarily prefer SDL immediate
presentation, then mailbox, with vsync as the unsupported-device fallback.
Every offscreen pass and draw callback is retained for rendering dependencies.
Fast embedded GPU submissions use a bounded fence queue, avoiding unbounded
GPU work/memory accumulation. Main-thread polling yields instead of sleeping
at a fixed rate during fast seeks; progress telemetry is throttled to 16 ms.

Audio is muted through SDL logical-device gain while seeking. Streams, volume
settings, decoders and voice state remain active; this does not reconstruct
audio position at the target. Seek completion, cancellation, stop and failure
restore gain. Non-TAS scheduling remains unchanged. No image-history cache or
serialized save-state is introduced.

## Windows delivery

- Game: `runtime-builds/modern-x64-tas-fast-seek-03/kinoko_modern_gpu.exe`,
  source `d64dac9d`. Fonts, shaders and all three DAT are staged beside it;
  DAT size and SHA256 checks passed.
- Editor: `C:/WorkSpace/6kinoko-tas/artifacts/windows-editor-20/KinokoTAS.App.exe`,
  source `57f7064`. Select the new game executable to enable unlimited seeking.
- Logs: `build-runs/modern-x64-tas-fast-seek-03/`; earlier 01/02 builds retained.
- `tas_edit_contract`, `replay_contract`, `replay_runtime_contract` and
  `audio_output_contract` passed. Real runtime/Squirrel edit scenarios use
  unlimited seeking, then verify the resulting RNG/checkpoints from fresh VMs.
- Hidden-window GPU contract passed: intermediate readbacks are suppressed,
  bounded GPU submissions finish, target and cancelled-frame pixels/labels
  agree. Fixtures remain under the build tree's `Testing/fixtures/`.
- Editor full synthetic checks passed in `artifacts/checks-51`, with build and
  publication logs in `artifacts/build-51`. New protocol, legacy fallback,
  unchanged playback speed and cancellation are covered.

No actual game gameplay or gameplay speed benchmark was performed. No new
Linux/macOS manual validation is claimed. Speed depends on the machine; the
change removes scheduling caps, not the cost of simulation/rendering itself.
