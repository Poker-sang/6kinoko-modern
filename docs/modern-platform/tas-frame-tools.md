# TAS frame tools and pacing — 2026-09-30

Engine source: 24cbb612.
Windows x64 game: runtime-builds/modern-x64-tas-tools-02/kinoko_modern_gpu.exe.
Build/logs: build-runs/modern-x64-tas-tools-02/.
All three DAT files are staged beside the executable and SHA256 verified.

KTASED02 explicitly encodes the original source length separately from the new
input length. The bridge verifies the prefix against the source, then records
the new suffix, including append-at-EOF and shorter recordings. KTASED01 remains
supported. Size, source bounds, action masks and truncated data are validated.

The speed command accepts 25/50/100/200/400 percent. The TAS worker uses SDL
nanosecond pacing instead of the ordinary frame-timer wait. The simulation clock
remains frame-based at 60 Hz; RNG and checkpoint generation are unchanged.
Normal non-TAS scheduling is unchanged. Audio remains on its original real clock.
Capabilities advertise edits-v1, edits-v2 and pacing-v1.

tas_edit_contract, replay_contract and replay_runtime_contract all passed.
Synthetic real-runtime/Squirrel scenarios shorten and extend recordings past
the old EOF and replay the resulting checkpoints from a fresh state. Pacing
checks confirm that scheduling does not advance the simulation clock.
No actual game gameplay or new Linux/macOS manual testing was performed.

Editor delivery: 6kinoko-tas/artifacts/windows-editor-18 (source 403a412).
Usage: https://github.com/Poker-sang/6kinoko-tas/blob/master/docs/frame-tools-delivery.md
All previous outputs and logs are retained.
