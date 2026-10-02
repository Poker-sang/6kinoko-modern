# Numbered rewards and classic-course presentation

Source/package snapshot: `67fc6d15`, new-stage 1.2.0, Windows x64.
Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-14/6kinoko-modern-windows-x64-67fc6d15/Launch-Mods.cmd`

The user confirmed delivery 13 was playable overall, then reported all red stars
counted as the first, the green balloon was missing, no transformations were
provided, and the course did not sufficiently resemble classic Mario 1-1.
Delivery 14 has not been gameplay-validated.

The recipe now uses red-star blocks 1420/1421/1422, selecting original indices
0/1/2. Blocks 1083 and 1086 select the original Init4Head and InitTeaSelect
transformations. A focused native contract executes the real original block
bytecode and verifies those selections rather than inferring rewards from IDs.
The route retains the opening blocks, growing pipes, three gaps, paired stairs,
final eight-step staircase and 704 pixels of road beyond the goal. It reuses
Kinoko physics and artwork; underground warp and exact Mario physics are absent.

The entrance moves to (64,832), one step above the house. Green balloon chip
1148 is placed at (32,768) within the first-world page, in the existing symbol
layer. The original state carrier still handles clear/unlock state; the green
chip mirrors visibility and receives original animation rectangles through a
scoped adapter. Neighboring balloons retain their original behavior. The HUD
wrapper retains explicit PlayerStatus.global binding from delivery 13.

All nine resources remain external in the Mod package, including added green
balloon/pipe textures, stage label, map, script and ACT/MCD overlays. Original
DAT archives are untouched. Existing original MCD chip records are preserved;
new definitions reference a full original solid-block image and an external
64-pixel pipe body derived from original pixels. Final offline terrain preview
confirmed the full block artwork and both pipe halves, without launching the game.

Checks and logs retained under build-runs/mod-classic-revision-01:

- resource-test-03.log: real-resource integration.
- editor-test.log: bounded ACT editor checks.
- script-test-03.log: real original HUD/block/world bytecode contracts.
- package-validation.log: manifest hashes, reproduction of all nine resources,
  and preservation of original MCD chip records.
- dat-package.log: all three DAT archives staged/checksummed beside the EXE.
- prepare.log: installed/enabled session preparation without game execution.
- final-terrain-preview.png and final_preview.py: final offline artwork inspection.

The optional native contract was freshly compiled in
build-runs/mod-classic-revision-03. The game executable itself is the unchanged
Windows x64 Release binary from runtime-builds/mod-new-stage-08; this delivery
changes Mod resources and tools, not native game code. Old artifacts remain.
