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
