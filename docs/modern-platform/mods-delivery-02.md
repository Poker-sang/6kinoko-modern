# Mod packages and managed profiles

Source: `39e0df299c5ed7875b91759c2f4a7b245f49b801` on `codex/mod-overlays`.
This batch adds deterministic `.kmod` packages, validated installation, exact
version/content selection, dependency-aware enable/disable/order, retained profile
history, and Windows install/launch scripts. It does not add a graphical editor.

Both replacement and entirely new `data/...` resources are supported. The archive
contract now reads `data/new.bin`, which does not exist in the mounted DAT, then
confirms it disappears when Mods are disabled. New gameplay content still needs
scripts/registries that reference these resources; asset installation alone does
not register levels, enemies, bosses or transformations.

## Local Windows delivery

Under `C:/WorkSpace/6kinoko-mods/runtime-builds/mod-windows-02/`:

- `6kinoko-modern-windows-x64-39e0df29/Launch-Mods.cmd`: sample already installed and enabled.
- `6kinoko-modern-windows-x64-39e0df29/Install-Mod.cmd`: drag a `.kmod` here to install and enable.
- `cyan-title-cursor.kmod`: original sample artwork packaged for distribution.
- `6kinoko-modern-windows-x64-39e0df29.zip`: clean distribution without original DAT or installed profile.

Python 3.10 or newer is required for Mod management. Ordinary `launch.cmd` starts
without the managed Mod profile. The three original DAT files are staged beside
both the raw and local packaged EXEs, with size and SHA256 verified.

## Validation

Committed source preceded the fresh `build-runs/mod-packages-02` build. Windows
x64 Release game and focused contracts compiled successfully. Both Python
session/package contract suites passed. Native `file_archive_contract` and
`mod_resources_contract` passed (2/2). Packaged sample installation and
`launch --prepare-only` succeeded; all 39 distribution manifest files matched.
Logs, fixtures, intermediate builds and packages are retained. No gameplay was
run by the agent. The previous cyan cursor visual result was confirmed by the user.

CI run `36591594631` was still running at delivery preparation; macOS/Linux full
game compilation and Windows Win32 checks had passed. Do not treat pending jobs
as passed. See the run for subsequent results.

## Integration boundary

No TAS bridge, replay, input, render or runtime-option interfaces changed in this
batch. Mods plus replay/record remain explicitly rejected pending identity
integration. The native identity API from the previous batch is unchanged.
The separate TAS checkout and rebuild repository are untouched.
