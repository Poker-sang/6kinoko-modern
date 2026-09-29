# Independent input actions (version 1)

Original `keyconfig.dat` remains compatible and unchanged. Without an action
configuration, every action inherits its original input counter and assignment,
including controller mappings and in-game key changes. This preserves hold/repeat
thresholds (including the pause menu's second-frame confirmation).

Copy `config/input-actions.split.cfg` beside the executable as `input-actions.cfg`
to try independent keys, or use the classic template and override only desired
actions. Restart after editing. No new in-game configuration screen is introduced.
A configured action replaces its legacy sources; omitted actions or `legacy`
continue to follow `keyconfig.dat`. Deleting input-actions.cfg restores inheritance.
The game never rewrites this configuration or replaces existing keyconfig.dat.

| Action | Original source / purpose |
|---|---|
| moveLeft/Right/Up/Down | Movement arrows; includes ladders, crouching and aiming |
| jump | Z / original button 0; jump and swimming jump |
| confirm | Z / original button 0; menu acceptance and demo advance |
| attack | X / original button 3; projectile attack |
| run | X / original button 3; running, jump speed, swimming speed and rebound |
| carry | X / original button 3; hold an enemy, release to throw |
| door | Up; enter normal/hidden doors |
| pipeUp / pipeDown | Up / Down; enter corresponding pipes |
| pause | A / original button 1; open pause |
| menuBack | A / original button 1; original pause-menu secondary action |
| useItem | C / original button 2; use stock item |
| menuLeft/Right/Up/Down | Arrows; menu and world-map navigation |

X is not a cancel key. MenuBack retains the original pause-menu behavior; it does
not add a universal cancel operation. Original any-button prompts and hidden-menu
chords remain valid. Different transformations of the same jump/attack are not
separate actions. The example separates keyboard controls; add `pad:N` explicitly
for overridden actions to retain a chosen controller button.

Syntax: `version = 1`, then `action = source, source`. Sources are `legacy`,
`key:SDL scancode name`, `scan:legacy scan number` (0..255), `pad:button index`
(0..31, any connected controller), or `none` alone. `#` starts a comment.
Unknown names, duplicate actions, invalid keys and unsupported versions reject the
whole file with a visible error and restore legacy defaults. Files use UTF-8.

## Implementation and future interfaces

`input_actions.hpp` exposes stable action IDs, binding parsing and deterministic
frame advancement independently of SDL and the game. Frame counters and falling
edges are separate from writable script publication; DisableInput cannot cause a
held custom key to appear freshly pressed next frame. A future UI may edit these
bindings; TAS may consume logical frames. Replay, recording and an editor are not
implemented by this change.

The original Input object retains x/y, b0..b5, k0..k5 and release aliases. New
named fields expose the independent counters; moveX/moveY and menuX/menuY are
signed axes. Door and pipeUp retain negative vertical-count semantics for original
checks. Script setters are mirrored during cutscenes and DisableInput; original
assignments, comparisons and branch targets remain effective.

Original DAT files are not edited. The bytecode loader clones function prototypes
and patches only verified input accesses, guarded by source/function and a hash
of instruction words plus ASCII string literals. Unknown variants fail visibly.
The generated manifest is derived from local inspect_cv4 JSON; it contains only
patch metadata, no game bytecode. Both archive variants are covered. New mod
scripts should directly use named fields. Dynamic text and settings UI are unchanged.

## Validation

Focused contracts cover inherited counters/device handoff, independent bindings,
multiple sources, releases, parser rollback, script input suppression and refusal
of unsupported original variants. Optional local CV4 arguments validate all
extracted originals without executing gameplay. Build/test delivery is recorded
separately; compilation is not user gameplay validation.
