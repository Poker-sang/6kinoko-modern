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

## Local delivery evidence

Source: `492441a936eaaff61f5771a77d5dedca55f6200f`.
- `modern-x64-graphics-02`: Release game built; GPU resource, graphics runtime
  and application contracts compiled separately (contracts-build.log).
- `modern-graphics-02`: Win32 Release game and all 84 contracts compiled.
- Both builds staged and SHA256-verified all three original DAT; shaders staged.
- Both static D3D9 dependency audits passed; x64 build summary has zero error
  diagnostics. No game or test executable was run.
- EXE: `runtime-builds/modern-x64-graphics-02/kinoko_modern_gpu.exe`.
- x64 EXE SHA256: `1c559c4a4ce92a219905ba77e36ffca736fefc2142eab83a183c021fac579c3a`.

The failed graphics-01 attempts are retained. Their compile errors exposed
callers relying on indirect Windows headers; graphics statuses/state types and
the ACT draw diagnostic counter now use project types/standard atomics. The
asset-name buffer remains 260 bytes, independent of OS path limits. The current
CharNextA text consumer explicitly includes its Windows backend until the font
migration rather than depending on an incidental graphics header include.

## Portable CI evidence

[Run 36456760951](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36456760951)
checked source `492441a936eaaff61f5771a77d5dedca55f6200f`. Ubuntu 24.04,
macOS 14 and Windows 2022 portable jobs all passed, compiling/linking the actual
game graphics backend and resource contract. The separate full-game cloud job
was still building at this snapshot; local full-game builds passed as above.
This evidence is compilation only, not graphical or gameplay validation.
