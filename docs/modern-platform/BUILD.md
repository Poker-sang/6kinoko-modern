# Build handoff

- Full Windows x86 Release source: `456743b4dcd365c2737dfb5e17111b5760037235`.
- Batch: `modern-02`. All default targets compiled successfully.
- EXE: `C:/WorkSpace/6kinoko-modern/runtime-builds/modern-02/kinoko_retdec_rebuild.exe`.
- Three original DAT copied next to EXE and size/SHA256 verified.
- Manifest: modern-02-artifacts.json (copy of build-runs/modern-02/artifacts.json).
- Logs and all outputs retained under build-runs/modern-02 and runtime-builds/modern-02.
- Windows x64 portable platform/codec modules compiled at source `bfa10551`
  (full commit in build-runs/portable-01/source-commit.txt); no execution.
- First Win32 batch modern-01 failed because the host library lacked SDL include/link
  propagation; fixed in 456743b. Failed build logs and outputs retained.
- No games, CTest, or local test binaries were executed. No new user gameplay result.
- New tests cover physical scan mapping, signed axis endpoints, window lifecycle,
  unfocused neutral input, relative consumption, and quit handling; compiled only.
- Portable CI results will be recorded separately. The complete game is NOT yet portable.

The game executable retains its inherited filename for compatibility with staging
scripts; it is a modern-fork build, not a replacement for rebuild binaries.

## Follow-up local batch: modern-03

- Source: `4d64b7f` (full SHA in modern-03-artifacts.json).
- All Windows x86 targets compiled; UTF-8 SDL title fix included.
- EXE: `C:/WorkSpace/6kinoko-modern/runtime-builds/modern-03/kinoko_retdec_rebuild.exe`.
- Three DAT copied and size/SHA256 verified. Quiet build, no gameplay or tests executed.
- All prior artifacts retained. 65 legacy contract executables plus new platform
  contract compiled; these counts are NOT test pass counts.

## Final local batch: modern-04

- Source: `e31d765f32522fba1736efc7ebe67bf967daec96`.
- Corrects startup controller publication before game manager/script initialization.
- All Windows x86 targets compiled, three DAT copied and size/SHA256 verified.
- Deliverable: `C:/WorkSpace/6kinoko-modern/runtime-builds/modern-04/kinoko_retdec_rebuild.exe`.
- Manifest: modern-04-artifacts.json. Full build logs retained locally.
- Quiet build; no game or test binary executed.

## Portable CI compilation

[Run 36314871824](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36314871824)
on source `1bd787f9`: Windows x64, Ubuntu 24.04 and macOS 14 portable platform and
codec targets all compiled successfully. See ci-platform-build.json for the
snapshot and exact head SHA. Contracts were compiled, not executed. The later
controller publication change affects the Windows bridge only; platform and
codec sources are unchanged. Windows complete-game CI was still in progress
at this snapshot; the final complete-game build succeeded locally as above.

## User feedback before audio migration

The user reported modern-04 looks normal on 2026-09-27. This is user gameplay
feedback, not agent-run validation and not a contract execution result.
