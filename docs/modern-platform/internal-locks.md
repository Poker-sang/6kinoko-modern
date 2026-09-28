# Portable internal locks (2026-09-28)

User accepted modern-x64-runtime-02; this is user feedback, not agent testing.

Graphics recursive synchronization uses an owned std::recursive_mutex and retains
nonblocking presentation and BeginScene/EndScene ownership. ACT runtime records
own a separate recursive mutex because their storage is allocated/copied through
native record views; initialization acquires ownership and disposal releases it.
Actor pool synchronization belongs to the native Pool object. Audio manager and
handle-table locks were never acquired: playback already uses AudioWorkers.lock,
so the redundant lock objects and their published vtable fields are removed.
The obsolete Windows CriticalLock helper is removed.

RuntimeRecord and audio manager memory layouts are native implementation details,
not DAT or save formats. Old x86-only layout assertions for the changed records
are removed; affected raw ACT/property/root-binding fixtures now use field offsets.
The portable services contract also compiles the actual graphics lock implementation,
covering recursive acquisition, exclusion on another thread, destruction and reconstruction.
No contract or game is executed by the agent.

Remaining full Linux/macOS blockers include Windows window/COM/IME hosting, GDI
font rasterization, ACT file enumeration and other native API/type dependencies.
This batch does not claim a full non-Windows playable build.

## Build evidence

Source commit: `a044dd231f1c9b6067b2dc0d6ffed9c891d80142`.
- `modern-x64-locks-03`: x64 Release game compiled; runtime-services (including
  real critical_section.cpp), audio and application contracts compiled separately.
- `modern-locks-03`: Win32 Release game and 82 contracts compiled.
- Both staged/hash-verified the three original DAT alongside EXE; shaders staged.
- Static D3D9 dependency audits passed. No game or contract executed.
- Source scan found no Windows critical-section types/calls in include,
  src/reconstructed, src/platform or src/squirrel (vendor sources excluded).
- Attempts locks-01/02 are retained: dynamic-layer calls and indirect Windows
  header/diagnostic dependencies were fixed before the successful locks-03 build.
- EXE: `runtime-builds/modern-x64-locks-03/kinoko_modern_gpu.exe`.

Every source correction was committed before its fresh build. All logs and prior
artifacts remain in build-runs/runtime-builds; this batch is pushed to modern only.
