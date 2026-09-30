# Fast seek rendering and publication optimization

Screen-only intermediate frames keep their CPU draw callbacks and latest
recorded packet but do not upload textures, execute screen GPU draws, acquire
swapchain images or download previews. Only one deferred packet is retained.
Target frames render normally. Cancelling renders the actual most recent packet
before acknowledging pause, for both embedded and external sessions.

Frames mixing screen and offscreen passes keep their exact pass order. These
passes may share depth or target history, so no offscreen dependency is dropped.
Offscreen-only frames also execute normally. Ordinary playback and gameplay
are unchanged. The deferred packet retains referenced images/pixel generations;
it is not an image-history cache or runtime save-state.

Fast seek replay output flushes every 256 frames and at the target. A paused
state publication flushes the stream before acknowledgement, covering cancellation
and takeover. Snapshot export and writer finish already flush. Checkpoints,
RNG, actions and wire bytes are unchanged. Normal live recording still flushes
each frame. Abrupt process termination during fast seek can lose the buffered
tail; published progress telemetry is not a durable-recording guarantee.

Synthetic validation covers target/cancel pixels at the Device queue boundary,
intermediate screen suppression, offscreen texture use after deferred frames,
pause flushing and existing runtime/Squirrel replay consistency. A synthetic
120-screen-frame timing comparison is logged; it is not a gameplay benchmark.

## Windows delivery

Source: `80271671`. Runtime:
`C:/WorkSpace/6kinoko-modern/runtime-builds/modern-x64-tas-fast-seek-04/kinoko_modern_gpu.exe`.
Release build, three replay/TAS contracts and both synthetic GPU contracts passed.
Logs and retained fixtures are in `build-runs/modern-x64-tas-fast-seek-04`.
The three DAT files were copied beside the executable and SHA256 verified.
The clear-only 120-frame synthetic comparison measured 165.087 ms normally and
1.278 ms with deferred rendering; actual gameplay speedup remains unmeasured.
Keep draw callbacks and mixed/offscreen passes, so logic-heavy scenes can gain less.
Select this executable in the editor and create/reopen the game session to use it.
