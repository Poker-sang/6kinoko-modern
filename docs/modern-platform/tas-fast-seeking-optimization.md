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
