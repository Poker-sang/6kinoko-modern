# Native input/audio boundaries and unused host cleanup (2026-09-29)

User accepted modern-x64-window-02 (user feedback, not agent testing).

The old IME path was disabled in Configuration (ime=0), its global enabled slot
had no writer, and its private text/composition buffers had no game-side reader.
No script API enabled it. Remove this unused IMM implementation, state, hooks,
fixtures and imm32 dependency declaration. Future editor text entry should use
SDL_StartTextInput/SDL text/editing events when an actual consumer is introduced.
This does not claim to implement a new text editor or IME feature.

The DirectInput implementation was already absent from the target source lists;
the game uses the SDL input bridge. Delete the unused backend and rename its
public interface to input_service.h. Initialization now uses the SDL host without
unused HWND/HINSTANCE arguments or a duplicate native-window cache. The actual
input service and its lifecycle contract are compiled on all portable CI hosts.
Input snapshots, key IDs, ranges and persisted binding widths are unchanged.

No active game source required COM activation after DirectInput/DirectSound moved
to SDL. Remove application/worker CoInitialize gates, matching CoUninitialize,
unused x86 COM adapters/CRT shim, and explicit game ole32 links. SDL remains
responsible for its own backend dependencies and per-thread initialization; this
is not a claim that the Windows binary has no OS COM/IMM imports from SDL itself.
The game worker still enters through kinoko_run_game_math.

Audio initialization drops unused HWND/options. Audio runtime uses fixed-width
integers and a packed 18-byte WaveHeader for the existing CV3 wire format, SDL
case comparison and bounded formatting, and no Windows SDK header. Reader-size
failure uses the reader's zero result, not a stale Win32 GetLastError. The audio
floating environment retains verified MSVC _fpreset behavior and uses standard
FE_DFL_ENV elsewhere. The full production audio runtime and existing ownership/
CV3/fade contracts now compile in portable CI, not merely the SDL output module.

Window entrypoint/modal-drag hook, native graphics aliases and GDI font rendering
still block full non-Windows game builds. No game or contract is executed by the
agent; all source/batches are committed before fresh builds and pushed to modern.

## Delivery evidence

Source `c9d29ba893f346589dd7075932b116ed531b0258`:
- `modern-x64-host-02`: full x64 Release game; input-service, audio-runtime and
  application contracts compiled separately (contracts-build.log).
- `modern-host-02`: Win32 Release game and 84 contracts compiled.
- Three original DAT staged and SHA256-verified for both runs; shaders staged.
- Both static D3D9 dependency audits passed. No game or contract execution.
- EXE: `runtime-builds/modern-x64-host-02/kinoko_modern_gpu.exe`.
- host-01 failures retained: final audio ZeroMemory/SetLastError dependencies
  were removed before the successful host-02 source commit/build.

All old binaries, logs and source commits are retained; source and this evidence
are backed up to the modern repository branch, never rebuild.
