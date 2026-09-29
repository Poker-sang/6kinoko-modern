# Normal-flow first-map replacement

Source/build: `4c81280d`, master. Windows x64 Release, no gameplay execution.

Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-07/6kinoko-modern-windows-x64-4c81280d/Launch-Mods.cmd`

Only first-stage-swap 1.0.0 is installed/enabled in this fresh directory. Follow
normal title/save/world-map flow and enter the first node. No stage chooser.
The second main-map bytes overlay data/map/w1-c01a.act without an entrypoint.
Player/HUD startup, lives, time, transitions and original DAT remain unchanged.
Mod-combination saves remain isolated. Original second-map submap destinations
are retained; this is not a complete remapped campaign or a new level yet.

Map SHA256: 4f3c43aa515c1a73cad20ef943f37ac2ecd22991e1e64091c59aaa0857b444f8.
Generated editable project and kmod sit beside the extracted game directory.
Clean ZIP excludes original DAT and generated map. Old deliveries remain intact.

Validation: committed before fresh build. Python stage_swap_mod_contract passed;
file_archive, mod_resources and mod_scripts contracts passed (3/3). These checks
cover resource delivery and API behavior, not actual gameplay. Exact replacement
bytes match the original second map. All 47 package manifest files verified.
Three DAT files copied and size/SHA256 verified beside both raw and packaged EXEs.
Prepared installed-Mod session without launching game. Logs and fixtures retained
under build-runs/mod-stage-swap-01. No manual play or CI success is claimed.

The user rejected the prior direct-start approach after missing player/HUD.
Existing logs show nextFace missing in SetFaceType during player Init, followed
by deadCount errors in camera updates. Original titlemenu UpdateMenu instructions
26..37 begin PlayerImage and PlayerStatus before normal level entry. The old
practice sample skipped this. It is now documented as broken/superseded rather
than used for future levels. No guessed player fields were added.

Next milestones: custom ACT terrain replacing first map, then separate new map
plus world node/unlocks/save identity, then additional worlds. Actual gameplay
confirmation of this first replacement is still pending.
