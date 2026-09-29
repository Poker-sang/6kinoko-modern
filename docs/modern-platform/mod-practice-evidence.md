# Practice-stage integration evidence

Original executable SHA256:
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
Local research and raw script IR are retained in
`build-runs/mod-playable-research-01`; original game script dumps are not published.

The IDA skill startup/open scripts opened an isolated temporary database.
Minimal survey and focused decompile of 469840 were saved. An initial no-analysis
decompile failed; explicitly defining the function enabled the focused result.
It confirms the script LoadMap wrapper delegates to the native map manager and
returns the low-byte truth result. No native loader logic is changed here.

DAT index decoding follows current `compat/archive_index.hpp` and standard
MT19937 (not the older unused archive.cpp temper-mask variant). The bounded local
builder reads a/b/c in mount order, checks bounds and decodes payload XOR. Known
MT19937 seed-5489 values are checked independently in the builder regression.

Original Squirrel wire structures/opcodes were read from bundled Squirrel 2.2.2
sqobject.cpp/sqopcodes.h. The research reader parsed the compiled resource scripts
without executing them. Evidence used:

- boot.nut imports stage/global/player/enemy APIs, loads UI ACT resources,
  calls InitStage("op.act"), then InitGlobal.
- stage.nut InitStage(stFile), instructions 10..24: fourth character `s`
  sets stageNoIntro; instruction 83 onward calls LoadStage(stFile).
- LoadStage instructions 43..46: loads `data/map/` + filename. Later it initializes
  collision, render layers, events and actor tables. Instructions 315..341 use
  the no-intro path, fade in and select UpdateGlobal.
- savedata.nut InitStageSaveData strips only the extension for arbitrary map
  filenames and creates missing per-map statistics in currentSavedata.
- titlemenu.nut AcceptTop selects savedata[slot].weakref for currentSavedata.
- demo/op.nut references TitleMenu, WorldMap, Logo, their stage players and Fader2.
- stage.nut ChangeStageToTitle owns fade, actor/render/map cleanup and re-enters
  op.act. global.nut UpdateDead normally calls ChangeStageToWorld after life loss.

The sample reuses these entrypoints instead of reproducing player physics or
inventing a replacement level loader. It supplies a new resource path and new
practice-mode script only; the copied map's terrain remains the original first map.
The regression exercises sample lifecycle with mocked original functions in the
real VM. It does not claim an in-game run or original map runtime validation.
