# SDL3 audio output migration

## Scope and evidence

User authorized DirectSound-to-SDL3 output migration after reporting modern-04
normal. Preserve decoder, SFL loop handling, request queues, fade scheduling,
per-ID sound restart, and game/codec floating-point scopes. Squirrel source VM
and bindings are unchanged; no Squirrel semantic migration is part of this batch.

Existing IDA evidence: docs/audio-scheduling/4099c0.json (ring refill/EOF/fade),
40a8d0.json (pause toggle), docs/typed-native-runtime/audio-ownership.md, and
src/decompiled/6kinoko.exe.c. Original import survey is retained in
../modern-platform/E-imports.json. Read-only MCP byte-anchor attempt/status is
recorded separately. No database write/decode operation or game was requested.

## Design

- Portable OutputDevice / OutputBuffer interfaces carry real owners and size_t
  capacities; no COM, HWND, Windows headers or 32-bit address slots in the backend.
- One SDL logical playback device, with independent SDL audio streams per voice.
  SDL mixes and converts PCM8 unsigned / PCM16 little-endian streams. The current
  assets/legacy bridge use these PCM formats; unsupported or unaligned formats
  fail explicitly. SDL may choose a native driver such as WASAPI on Windows.
- Pause unbinds ONLY that stream, retaining queued PCM and the source cursor.
  Resume binds it again. Explicit seek clears queued samples and resets the ring.
  Never pause the shared audio device to implement a single BGM pause.
- Ring operations serialize with SDL's stream lock, also held for callbacks.
  Control operations bind/unbind outside that lock to respect device/stream lock
  ordering. The callback only copies bounded PCM blocks, never decodes DAT/Ogg or
  calls into the game. No game locks, allocation of application buffers or file I/O
  occurs in the callback (SDL's own queue management may allocate internally).
- A source-submission cursor replaces hardware play/write cursors at the existing
  ring refill boundary. It denotes PCM already copied to SDL; it is not an exact
  DAC playback timestamp. Backend buffering/resampling can shift audible latency.
- Original gain-to-hundredths-of-dB formula is retained; the output backend converts
  those dB to amplitude with 10^(dB/2000), with -10000 as silence. Treating the
  original gain directly as SDL amplitude would change the volume curve.
- Streams retain shared device state; destroy streams before closing device and
  SDL audio subsystem. Runtime still joins its Win32 loader/update workers first.
- The old unused primary/device/listener integer/pointer aliases are removed.
  Shutdown returns device-existed status instead of a truncated device address.

## Verification boundaries

New portable contracts cover ring wrap, frame alignment, unsigned silence,
one-shot exhaustion, dB conversion, two independently paused voices, seek,
resume and buffer/device lifetime. Existing ownership/CV3/fade/queue/rounding
fixtures are adapted to the output interface, not replaced by no-op stubs.
The optional sound-module fixture now opens SDL instead of dsound.dll.
All contracts are to be compiled only. No local tests or gameplay are run.

The output backend is portable; audio_runtime.cpp still uses the inherited
Win32 workers, critical sections, timer and CV3 WAVEFORMATEX parsing boundary.
That high-level service migration remains separate. Full game remains Windows x86.
Do not claim waveform, audible latency, or gameplay parity from compilation.
