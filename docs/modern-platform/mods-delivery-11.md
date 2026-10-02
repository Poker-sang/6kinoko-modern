# Solid earth boundaries and road after the goal

Source/package snapshot: `72a04f86`. User reported delivery 10 still incorrect
and supplied a screenshot of the step course, then requested the original-style
road after the goal balloon. No gameplay success is claimed for this delivery.

Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-11/6kinoko-modern-windows-x64-72a04f86/Launch-Mods.cmd`

Default enabled Mod: new-stage 1.0.2, normal world flow, up from Marisa's house
to node 16. Corrected short-course 1.0.2 replacement is packaged separately,
not enabled by default. World entrance and original stages are unchanged.

## Root cause and correction

Delivery 10 incorrectly selected the pass-through earth-platform family.
Original MCD flags for caps 1025/1027 are 0x20 and for walls 1028/1029 are
0xf0. These are excluded from side contacts; restoring their artwork cannot
provide solid cliff collision. Original solid cliff caps 1030/1032 and walls
1033/1035 have flags zero, and original w1-c01a uses these on its cliff edges.
The generator now uses this existing solid family on exposed boundaries,
retaining interior filler and continuous base grass. Native physics is unchanged.

IDA MCP original 4674B0 confirms the side-contact exclusions: mask/compare 0xf0
at 4674DA/4674E1 and mask 0x30 at 46759C. Raw evidence is retained under
build-runs/mod-solid-earth-research-01, including the initial unanalysed read
and successful instruction decoding. No proprietary evidence is committed.

Original first stage is 6720 wide with its goal at x6016, a difference of 704.
The authored goal remains x2304 and map width is now 3008, matching that spacing.
The final flat floor extends to the new right boundary. Both Mod generators
share this recipe and fix. Map stage SHA256:
7cdd8c16a65accf556b3e909e9657da968a56b191185d9ecee0ed5b2c71ac493.

## Validation and retained artifacts

Committed before checks. ACT editor and real-resource integration passed:
solid cap/wall selection, actual MCD flags, buried/base grass, goal-tail spacing
and traversable floor placements, preserved world resources and session creation.
Package hashes verified; packaged generator reproduced both delivered resources.
Required three DAT files copied and checked beside the packaged EXE.

This is a resource/Python-only update. The package reuses the unchanged Windows
x64 Release binaries from runtime-builds/mod-new-stage-08; no new native build
or gameplay execution occurred. The package's source snapshot identifies its
updated tools and recipe. Prior native contract results remain in delivery 10;
they were not rerun this round. Current logs/fixtures are under
build-runs/mod-solid-earth-research-01 and build-runs/mod-solid-earth-01.
All prior deliveries and artifacts are retained. In-game visuals, side blocking
and goal completion still require user verification.
