# Mod authoring tools and main-branch integration

User authorized merging the isolated Mod branch. PR #1 merged into `master` as
`2ea7564b2cec42eb86c23d3c3c87636d3b50bff8`. The TAS working directory and branch
were not modified. The previous CI run 36591594631 completed all compilation and
test steps successfully; its only failed step was Ubuntu portable artifact upload
(`FinalizeArtifact` request timeouts). This is not an all-green workflow result.

Authoring source/build: `1960e02d` on master. Adds project initialization, manifest
maintenance, raw file import, PNG-to-32-bit-CV2 import, CV2-to-PNG export and project
validation. Pillow is optional except for PNG operations. No gameplay registration
or automatic script execution is introduced. See [usage](mods.md#creating-new-resources).

## Delivered files

Base directory: `C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-03/`.

- `6kinoko-modern-windows-x64-1960e02d/Launch-Mods.cmd`: authored cursor installed/enabled.
- `6kinoko-modern-windows-x64-1960e02d/mod_author.py`: bundled authoring CLI.
- `authored-cursor/`: editable sample project generated using delivered tools.
- `cursor-source.png`: exported original sample artwork, usable in image editors.
- `authored-cursor.kmod`: package created from the imported PNG.
- `6kinoko-modern-windows-x64-1960e02d.zip`: distribution without proprietary DAT.

The sample still renders the cyan title cursor; no new in-game content is claimed.
New-resource paths are covered independently by package and native archive tests.

## Validation

Source committed before the fresh `build-runs/mod-author-01` build/tests. Windows
x64 Release compiled. All three Python suites passed, including exact PNG channel,
alpha and dimension round-trip, refusal to overwrite, invalid-path rejection,
empty-draft rejection and project-to-package-to-installed-profile integration.
Native file_archive/mod_resources contracts passed (2/2). Delivered tools exported
the sample, created a project, imported PNG, packed, installed and prepared launch
without executing the game. All 40 package manifest hashes matched. Three original
DAT files were staged beside raw and packaged EXEs and size/SHA256 verified.
All logs and fixtures remain under the build directory. Gameplay is user-owned.

A follow-up CI-only correction explicitly propagates each Python command failure
instead of allowing a later successful command to mask it in PowerShell. This
does not change the delivered 1960e02d game or tools. New CI outcomes are not
claimed by this delivery record.
