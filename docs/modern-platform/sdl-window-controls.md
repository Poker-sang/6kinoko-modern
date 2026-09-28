# SDL window controls (2026-09-29)

User accepted modern-x64-directory-03 (user feedback, not agent testing).

Alt+Enter is consumed as an SDL key event for the game window and applied on the
main thread through the existing graphics reset/listener sequence. Key repeat is
ignored so holding the shortcut does not repeatedly toggle. SDL owns desktop
fullscreen and window restoration; the old Win32 border-metric repositioning and
unused cached native window style are removed. On returning to windowed mode SDL
restores the prior placement, rather than forcing the old Win32 recentering.

FPS titles are UTF-8, queued from the game thread and applied by the main SDL pump.
Cursor visibility, startup show/minimize/maximize, graphics client-size queries,
and application/graphics/audio initialization and GPU-error dialogs use SDL APIs.
The application still maps the incoming Windows launch hint to SDL operations.
The GPU state no longer caches HWND solely for its error dialog. Graphics trace
counters now use standard atomics. Only close events for the owned SDL window
request game shutdown; global SDL_QUIT remains accepted.

The Windows subclass is retained narrowly for the existing IME boundary and
move/size modal-loop GPU pumping, preserving the earlier window-drag rendering
fix. Its elapsed clock now uses SDL_GetTicks. COM, the entrypoint/single-instance
handle, legacy text dialogs, native graphics type aliases and GDI fonts still
need migration. This does not produce full Linux/macOS games yet.

The platform contract covers queued UTF-8 title updates from a worker, one-shot
fullscreen requests, key-repeat/foreign-window filtering, and close/reopen state.
Contracts are compiled only. Gameplay/fullscreen/window-drag verification remains
with the user. No DAT, save format or gameplay update rules are changed.
