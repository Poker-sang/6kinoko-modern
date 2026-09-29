# TAS edit-plan closeout

Engine source: 2df218fb.
Windows x64 Release: runtime-builds/modern-x64-tas-edits-02/kinoko_modern_gpu.exe.
Build and logs: build-runs/modern-x64-tas-edits-02/.
All three DAT files staged beside the executable and SHA256 verified.

The TAS bridge publishes edits-v1 and loads an optional KTASED01 input plan.
The original recording prefix is verified, then control changes to recording
at the first edited frame (including frame zero). The engine regenerates held
counts, release edges, RNG state and checkpoints; physical input cannot alter
the scripted suffix. Input masks, range, length and trailing bytes are validated.
Normal game and replay invocation without an edit plan retains existing behavior.

Editor implementation and usage:
https://github.com/Poker-sang/6kinoko-tas/blob/master/docs/editing-closeout.md

tas_edit_contract, replay_contract and replay_runtime_contract passed locally.
The real replay runtime/Squirrel synthetic contract executes edited input from
frame zero and after a verified prefix, then replays the new recording in a fresh
VM and verifies all checkpoints. This is automated synthetic validation, not
actual gameplay. No Linux/macOS manual tests were performed.
