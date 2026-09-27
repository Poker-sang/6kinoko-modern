# Audio build handoff

Final local source: `83bc67cae52ba71deb150f4b1aba00511849a884`.

- Batch: modern-audio-03, Win32 Release, diagnostics quiet.
- EXE: `C:/WorkSpace/6kinoko-modern/runtime-builds/modern-audio-03/kinoko_retdec_rebuild.exe`.
- All targets compiled, including the 65 inherited contracts and the portable
  platform/audio output contracts. No contract executable or game was run.
- Three DAT files copied next to the EXE and size/SHA256 verified.
- Artifact hashes: modern-audio-03-artifacts.json. All build outputs/logs retained.
- modern-audio-01 failed on obsolete COM initialization after removing dsound.h;
  the obsolete worker COM gate was removed. Failed artifacts retained.
- modern-audio-02 compiled and staged successfully, before the one-shot seek fix;
  its manifest is retained separately. Do not substitute it for modern-audio-03.
- Windows x64 portable backend/contracts compiled at `315756bb` in
  build-runs/portable-audio-01. The later one-shot fix requires current CI compilation;
  no local test binary was run.
- modern-04 (preceding platform batch) was reported normal by the user. This is
  user feedback only, not agent-run gameplay or a passing contract suite.

Portable Windows x64/Linux/macOS CI is compile-only. Results are recorded separately
once available. Complete-game portability, audible parity and latency are not
established by these builds. Native rendering remains D3D9; audio scheduling
workers still use Win32 services. Next planned major backend is rendering.
