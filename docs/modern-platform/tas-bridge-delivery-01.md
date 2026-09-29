# TAS bridge delivery — 2026-09-29

Game source: `94aefb79`, branch `codex/tas-bridge`, based on `65b2eb01`.
Editor source: `2981a59` in independent local `C:/WorkSpace/6kinoko-tas`.
Editor package: `C:/WorkSpace/6kinoko-tas/artifacts/windows-editor-04/KinokoTAS.App.exe`.
Game package: `runtime-builds/modern-windows-tas-bridge-01/6kinoko-modern-windows-x64-94aefb79/kinoko_modern_gpu.exe`.

The editor shows offscreen SDL GPU frames from an owned hidden game process.
KTAS1 file mailboxes provide frame-boundary pause, target, run, takeover and stop.
Takeover preserves the verified replay prefix and appends live logical input to
a new branch. Each session owns isolated initial saves and per-run writable saves.
Normal startup enables neither the bridge nor GPU readback. Original recordings
and normal saves remain untouched. New recordings start with empty saves.

Backward seeking restarts from initial saves and simulates forward at normal speed.
There are no state snapshots, instant rewind or fast-forward yet. Timeline edits
remain .ktas project intent; they are not injected into replay. Save a live branch
before reopening it for backward seeking. Readback is synchronous and the file
transport may limit real-time performance. Music is not yet tied to tool pause.

Validation (no actual gameplay run by the agent):
- Build and replay/runtime contracts passed in `build-runs/modern-x64-tas-bridge-03`.
- Runtime contract verifies prefix, takeover release edge, branch finalization,
  and subsequent deterministic playback through the actual replay runtime.
- Synthetic hidden SDL GPU contract verifies RGBA readback and frame label 42.
- CI run https://github.com/Poker-sang/6kinoko-modern/actions/runs/36590053303
  passed all seven jobs, including Linux/macOS compilation and Windows contracts.
- Delivered package manifest hashes and all three original DAT hashes verified.
- Editor build-10/checks-10 passed file/edit, fake-process protocol and headless
  UI checks. Its inspected screenshot is synthetic, not a game screenshot.
- Editor publish requires .NET 10. See its bundled 使用说明.md for controls.

All build artifacts and logs are retained. Mod work stays on its independent
branch; this delivery does not merge it or claim Mod replay compatibility.
