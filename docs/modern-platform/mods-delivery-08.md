# Authored short-course replacement

Source/build: `aa0c6003`, master. Windows x64 Release; no game execution.

Launch:
`C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-08/6kinoko-modern-windows-x64-aa0c6003/Launch-Mods.cmd`

Fresh directory has only short-course 1.0.0 enabled. Use normal title/save/world
flow and enter the first node. User confirmed the previous second-map replacement
worked; this new authored course has not yet received gameplay confirmation.

The course is 2560 pixels wide with three increasingly high step groups, two
original enemies, spawn at (160,832), and goal at (2304,832). It contains 520
terrain placements. Art/background and behavior remain original, while layout
is authored. Warp/hidden/old enemy placements are removed. The original first-map
script is preserved byte-for-byte, including its dormant warp callback. Normal
player/HUD lifecycle remains intact. Mod saves are isolated as before.

The editable JSON recipe is examples/mod-authoring/short-course/map.json and is
also packaged under mod-authoring/short-course. Use --recipe with the generator
and a fresh --output directory. The generated project and short-course.kmod are
beside the extracted game. Original DAT/generated map are not in the clean ZIP.
Map SHA256: 62fef7917ef28bf09ed529b1a9bf25a0245a54d00e579de6f411760d492913ef.

## Evidence and limitations

The bounded ACT editor follows act_document.cpp/act_texture_io.cpp property and
placement serialization: sorted property names, per-type schema reuse, layer/key
hashes, count/size-prefixed placement arrays. Only self-described map-only ACT1
with 12-byte placements and chip resources is supported. Unsupported timelines
or layouts are rejected. No-op roundtrip preserves exact bytes. Edited cells
are sorted by x/y; placement bounds/count and document width are updated.

IDA session 01c07cb2 inspected original map schema registration at 434760.
The decompiler warned about stack analysis; field structure is cross-checked
against the recovered native reader and original resource parsing. Evidence is
retained under build-runs/mod-custom-map-research-01. Original MCD definitions
confirm 64x64 spawn/goal and 32x32 terrain/enemy chips. No physics or loader change.

## Validation

Source committed before build/tests. Windows build succeeded. Python editor
contract passed: identity roundtrip, count resizing, ordering, bounds, preserved
script bytes, malformed streams/edits rejected. Original ACT identity roundtrip,
new course dimensions/events/background and actual chip definitions checked.
Invalid width, floor gaps, missing goal and unknown chips rejected before output.
Native file_archive/mod_resources/mod_scripts checks passed (3/3). Package
manifest hashes verified; packaged generator reproduced the delivered map.
Three DAT files staged beside raw and packaged EXEs with size/SHA256 checks.
Installed Mod session prepared without game execution. Logs and fixtures remain
under build-runs/mod-short-course-01. All earlier artifacts retained.

These checks do not establish in-game movement, collision, goal completion or
return-to-world success. Next milestone after this course is confirmed: separate
new stage with its own world node, unlock rules and save identity.
