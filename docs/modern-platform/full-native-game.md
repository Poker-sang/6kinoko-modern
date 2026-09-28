# Full native game migration (2026-09-29)

User requests continuing until non-Windows platforms can run the complete game.
User accepted modern-x64-graphics-02. Most visible text is believed to be image
assets; preserve dynamic text for future localization rather than deleting it.

Work in this batch:
- Enable full native Linux/macOS targets and CI; retain Windows x86/x64 builds.
- SDL startup and error dialogs; native move/resize hook remains Windows-only
  because that OS modal loop blocks ordinary SDL pumping.
- Portable compiler conventions preserve explicit receiver/unused arguments.
- Fixed-width status/state values, standard atomics/formatting/bounded copies.
- Portable CP932 character traversal and generated Unicode map for original DAT.
- Shared stb_truetype + OFL Noto Sans CJK JP font assets; no system font discovery
  or GDI dependency. Original face names use the bundled fallback, so glyph
  metrics can differ. Text layout/atlas ownership APIs remain available.
- GLSL compiled offline to SPIR-V for Vulkan and MSL for Metal; retain Windows
  DXBC. The game stages and selects the appropriate shader format.
- Portable floating rounding, path splitting and diagnostics. Windows crash
  capture stays native; other platforms retain traces without pretending to
  provide Windows SEH/minidumps. VirtualQuery address validator had no caller.
- Remove unused process context/native-window slots. Multiple instances are no
  longer rejected through a Windows-only named mutex.

All builds must record committed source, use fresh directories and retain logs.
No game or contract is executed by the agent. CI packages must not redistribute
original game DAT; users copy their three DAT beside the packaged executable.
Full build success alone does not establish gameplay correctness.
