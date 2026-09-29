# Independent input delivery — 2026-09-29

Source: `4f27dee43a6626d4b4c4585627fa93c4c04f80ca`.
Branch: `codex/sdl-platform`, remote: Poker-sang/6kinoko-modern only.

## Behavior

19 stable logical actions with independent binding configuration. Original
`keyconfig.dat` loading/saving and the existing settings screen are preserved.
Unspecified actions inherit original assignments, including changes made in that
screen. Explicit action overrides remain independent. No additional UI is added.
See [configuration and action semantics](input-actions.md).

The original compiled scripts are adapted in memory with verified per-function
fingerprints. Reads and scripted assignments are both covered: attack/run/carry,
jump/confirm, door/ladder/pipes, pause/menu secondary action, item use and menu
navigation. X has no cancellation binding. Cutscene auto-walk and DisableInput
mirror script writes while persistent hardware counters remain untouched.

## Evidence

- WSL actions contract: passed on d46380c9.
- WSL script contract: passed on d46380c9, including all 141 locally extracted CV4
  files (58 distinct guarded function variants). No game code was played.
- Retained WSL evidence: `build-runs/input-actions-contract-02/`.
- Initial CI 36565189566 passed x64/Linux/macOS and portable jobs; Win32 rejected
  an obsolete fixed host-size assertion. 4f27dee4 removes only that assertion,
  retaining original input member offset checks and native storage-size checks.
- Final CI: all 7 jobs passed, with 8 focused contracts per full-game platform
  (32 passes). https://github.com/Poker-sang/6kinoko-modern/actions/runs/36565901938
- Retained initial CI downloads: `build-runs/input-actions-final-{windows,linux,macos}/`.
- Final artifact downloads stalled through the existing proxy and a direct retry;
  incomplete downloads are retained. User then requested Windows-only testing.
  A fresh local Windows x64 build provides the deliverable.
- Local DAT staging manifests: `build-runs/input-actions-delivery/`.

Gameplay and the optional split preset have not been user-validated in this batch.
Automated contracts do not establish gameplay validation.

## Windows delivery

- Local build tree: `build-runs/modern-x64-input-actions-local-01/`.
- Executable: `runtime-builds/modern-windows-input-actions-01/6kinoko-modern-windows-x64-4f27dee4/kinoko_modern_gpu.exe`.
- Launcher: `launch.cmd` in the same directory; tracing is disabled.
- Local archive: `runtime-builds/modern-windows-input-actions-01/6kinoko-modern-windows-x64-4f27dee4-with-data.zip`.
- All packaged file hashes and the three DAT copies were verified. DAT are also
  staged beside the intermediate local executable. No gameplay was run.
- The local package accurately records `tests_run: false`; successful same-source
  CI contracts are recorded separately in local-dat-manifest.json and above.
- Templates: `config/input-actions.classic.cfg` and `config/input-actions.split.cfg`.
  Copy a template to `input-actions.cfg` beside the executable, then restart.
  The package deliberately ships without an active override configuration.
- To retain existing custom keys, copy your current `keyconfig.dat` beside the new
  executable. Never overwrite a user-created file as part of staging.

User requested Windows-only testing at delivery. No Linux/macOS user tests are
requested. Prior successful automated cross-platform results remain historical
evidence; future user validation focuses on Windows.
