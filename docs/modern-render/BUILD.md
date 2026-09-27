# Build handoff

The first complete Win32 Release build, modern-render-01, succeeded at
c9a0fa6dcc540b4f2aa8a71d6d61682c92366fcd with all three DAT hashes verified.
Game and contract executables were not run. Its artifacts/logs are retained.

A follow-up makes the header transition table an inline C++17 variable so the
shared inline algorithm refers to the same table in every translation unit.
The final build is modern-render-02; its exact source and hashes will be copied
from the build manifest after successful compilation/staging.

Final Win32 Release all-target build succeeded at
`27ca62f3ae5a40f8ac5e02676adfe5b66d52ed83`.
EXE: `runtime-builds/modern-render-02/kinoko_retdec_rebuild.exe`.
All three DAT files are beside the EXE with size/SHA256 verification complete.
The manifest is `modern-render-02-artifacts.json`; its contract count covers the
65 legacy tools-directory targets. The new portable render blend contract also
compiled in the runtime directory, as did the existing portable modules/contracts.
Neither tests nor the game were executed. Logs and both build trees are retained.
D3D9 is still the active renderer; no full-game x64/Linux/macOS claim is made.

## Checkpoint 2: modern-render-03

- Source: `09718d7a91fac064acf15b7116075fcee0fac1f0`.
- Full Win32 Release all-target build succeeded; trace disabled.
- EXE: `runtime-builds/modern-render-03/kinoko_retdec_rebuild.exe`.
- Three DAT copied beside the EXE and verified by size/SHA256.
- Manifest: `modern-render-03-artifacts.json`. Its 65-contract count covers the
  legacy tools directory; portable contracts (including render_resources_contract)
  also compiled in the runtime directory.
- Logs retained: `build-runs/modern-render-03/{configure,build,dat}.log`.
- No game execution or local test execution. D3D9 remains the active renderer.
- User feedback: modern-render-02 currently looks normal; not agent verification.
- Previous checkpoint CI run 36317029201 completed successfully on all jobs;
  this does not establish checkpoint 2 CI results.
