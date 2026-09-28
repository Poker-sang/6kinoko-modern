# Portable runtime services (2026-09-28)

User confirmed modern-x64-files-02 works on Windows. This is user feedback, not agent testing.

Application update/display/load/retirement and both audio workers now use owned C++ threads,
recursive mutexes, atomic stop state and portable coalescing events. The frame timer uses the
same services and retains its 16 ms cadence and skip-on-busy registry behavior. Closing an
event wakes waiters; shared event ownership prevents unregister from freeing a pending wait.
Shutdown wakes workers and joins them before releasing their resources. Game math initialization
remains on the game worker. SDL thread priorities are best effort; low priority need not map
exactly to Windows IDLE. The event wait domain lives until process exit for static teardown.

Clock calls for scripts, ACT, audio and application use one service. Windows retains WinMM
uptime and 32-bit wrap; other platforms use SDL ticks. Disk/save formats are unchanged.
Audio worker startup returns success, no longer exposes a private Windows event handle.

The portable contract covers auto/manual reset, coalescing, wait selection, signal/close wakeup,
thread joining and frame registration. Compile only; neither contracts nor game run by agent.
Full Linux/macOS games still require Windows host/COM/IME/font and remaining internal service
migration; portable module compilation alone is not a playable game release.
