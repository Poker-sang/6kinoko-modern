# Classic 1-1 inspired course and green world balloon

Source/package snapshot: `e9605f48`, new-stage 1.1.0, Windows x64.
Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-12/6kinoko-modern-windows-x64-e9605f48/Launch-Mods.cmd`

Follow normal title/save/world flow, move up from Marisa's house, then accept at
the green balloon. The original first stage remains available. New save identity
remains w1-c16a; the lower world status panel now shows original-style Stage 16.

The user confirmed delivery 11 looked much better. This is user feedback, not
agent gameplay validation. This new layout has not yet been played by the agent.

## Course

Classic Super Mario Bros. 1-1 inspired progression: opening blocks/enemy, four
increasing pipe-like obstacles, first pit, elevated brick run, second pit,
mid-course block/enemy groups, paired stairs, third pit, two late obstacles,
eight-step final staircase and goal. Width 7040, floor y896; start (160,832),
goal (6336,832), post-goal road 704. Assets, enemies, block/item behavior and
physics belong to Kinoko. This is an adaptation, not pixel-exact Mario gameplay.
The route has no underground warp. The recipe is
examples/mod-authoring/classic-1-1/map.json, also copied into the package.

## World presentation

The ordinary original balloon 1023 has the same node-relative placement as
the original first entrance. The original numeric marker is replaced. Original
red/blue/yellow balloon textures remain unchanged. A separate symbol_mod layer
and MCD resource render the new node green, with original shading/alpha and
animation frames preserved. Its original symbol retains world visibility state,
while the green display mirrors visible and suppresses only that symbol's alpha.
This isolates the color from other world nodes.

The original PlayerStatus.SetWorld lacks a c16a label case. The presentation
entrypoint delegates original label/world selection and then associates a new
Stage 16 texture, restoring the c16a identity. The label reuses original word
and digit pixels. Original status ACT layers/resources are preserved, with one
new texture resource appended. No game startup or stage-transition hooks.

Original Japanese MCD texture paths are read by the local authoring helper as
CP932. The bounded ACT editor can append a map layer and explicit chip/texture
resource schema, validating the resulting document before publication.

## Verification and limits

Committed before checks. ACT editor and real-resource integration passed:
preserved world/status objects, gap cliff edges, course landmarks, independent
save resource, goal-tail spacing, balloon resources and immutable session.
The native optional contract was compiled in build-runs/mod-classic-03 and
executed actual original world/savedata/move/playerstatus/effect bytecode with
narrow API fixtures. Labels restore on original nodes and blank/house states;
both layers receive original animation calls, and simulated original visibility
changes synchronize the green balloon without changing its neighbor. This is
not proof of an in-game clear/unlock transition. Initial fixture attempts and
their diagnostics remain alongside successful script-test-07.log.

Label/green balloon previews were inspected from decoded CV2 pixels. Package
manifest hashes verified; packaged authoring tools reproduced all seven Mod
resources. Three DAT files were copied/checksummed beside the packaged EXE.
The game EXE is the unchanged previously compiled Windows x64 Release binary
from runtime-builds/mod-new-stage-08. No game execution or new native game build.
Artifacts/logs are retained in build-runs/mod-classic-research-01 and the numbered
mod-classic fixture/build directories. Old packages are retained.

In-game pacing, jumps/pits, actor placement, green-layer visibility and goal
completion require user verification. Original campaign totals/ending policy
remain unchanged; additional worlds and arbitrary node IDs remain future work.
