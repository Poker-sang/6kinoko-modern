# Portable game graphics interface (2026-09-29)

User accepted modern-x64-host-02; this is user feedback, not agent testing.

The game graphics API now owns its signed 32-bit Result/status helpers and
PixelRect geometry. State values, reference counts, pitches and dimensions use
fixed-width integers rather than Windows SDK aliases. Presentation is runtime
configuration, not a serialized record; its flags use bool and it no longer
stores a native window handle. Actual SDL window ownership stays in the host.

Remove ignored HWND/region arguments to presentation, ignored HWND arguments to
device construction, and unsupported shared-buffer HANDLE output parameters.
Update mesh/font/application consumers together. Texture regions, success/failure
sign semantics, ownership and render command values are unchanged.

kinoko_game_graphics builds the actual game device/texture/buffer implementation
once for the full game and portable CI. The existing GPU resource contract now
links this library on Windows, Linux and macOS, instead of being Windows-only.
The graphics runtime contract includes real public declarations on every host
and checks status/geometry widths and sign semantics for LLP64 and LP64.

Remaining full-game blockers include GDI font rasterization and font assets,
native application entrypoint/modal hook, shader staging/selection (the game
currently loads DXBC), and residual Windows consumers. Compiling the backend on
other platforms does not yet establish a playable non-Windows game. No game or
contract execution is performed for this batch.

User notes most visible game text is image-based, but asks to retain text support
for future localization. Keep layout, glyph caching and rendering interfaces;
do not delete text functionality based on that observation. GDI and CharNextA
remain explicit font-backend migration work, including checking actual callers
and choosing a distributable font/metrics policy before switching rasterizers.
