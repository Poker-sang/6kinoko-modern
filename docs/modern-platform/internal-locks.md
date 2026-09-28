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
