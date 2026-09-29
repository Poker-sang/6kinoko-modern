# TAS master integration — 2026-09-30

User authorized merging the TAS bridge into master. Merge commit 33f2b095
combines codex/tas-bridge with origin/master 2c1686c2 (including Mod work).

Windows x64 Release game and four focused contract targets built successfully.
replay_contract, replay_runtime_contract, mod_resources_contract and
mod_scripts_contract all passed. No gameplay was executed for this batch.
Three original DAT files were staged and SHA256 verified.

- Build/logs: build-runs/modern-x64-tas-merge-01/
- Game: runtime-builds/modern-x64-tas-merge-01/kinoko_modern_gpu.exe
- Editor: https://github.com/Poker-sang/6kinoko-tas (master)

The independent editor includes live timeline, takeover, bookmarks, replay-all,
and restart-after-exit. Input editing re-simulation and savestates remain future
work. Existing recordings and all build artifacts are retained.
