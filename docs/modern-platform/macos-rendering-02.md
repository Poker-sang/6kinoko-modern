# Metal upload stride correction

The user rejected rendering candidate `32dc959a` and supplied
`D:/录制于 2026-09-29 12.43.38.mp4` (37.167 seconds, 2940x1912).
Commit `444faba` reverts its depth quantization and HiDPI changes. Historical
packages and records remain; the prior depth hypothesis is not retained as a fix.

## Concrete cause

`Renderer::create_texture` uploads RGBA rows padded to 256 bytes and supplies
that layout in `SDL_GPUTextureTransferInfo.pixels_per_row/rows_per_layer`.
The vendored SDL Metal `METAL_UploadToTexture` ignored both fields: it computed
source row and slice strides from the destination region dimensions instead.
Consequently padding was read as pixels and following rows shifted. This
explains why some large/aligned textures render correctly while digits, door
animation, particles and the world map are striped. Windows handles the layout.

Original map `data/WorldMap/w1a.cv2` is 544x384: 2176 bytes of RGBA pixels per
row, but the transfer buffer uses 2304-byte rows. A static illustration reads
the original DAT with each addressing rule. The old rule reproduces the
recording's horizontal forest stripes; the descriptor rule restores the map.
This is asset inspection, not a game/GPU test. Evidence remains local under
`build-runs/macos-render-investigation-02/`, including video crops, script,
`worldmap-stride-evidence.png` and its metadata. Proprietary pixels/video are
not uploaded to the repository.

Additional source dimensions agree with the affected UI/animation:

| Asset | Extent | RGBA row bytes | Upload row stride |
| --- | --- | --- | --- |
| `data/System/Title/title_menu_num.cv2` | 160x20 | 640 | 768 |
| `data/System/stage_num.cv2` | 120x16 | 480 | 512 |
| `data/Actor/Item/op-door_0000.cv2` | 65x100 | 260 | 512 |
| `data/WorldMap/w1a.cv2` | 544x384 | 2176 | 2304 |

## Change and regression coverage

Fix SDL's Metal backend to respect the public transfer descriptor, including
the documented tightly packed default when either layout field is zero.
Compute slice pitch using the format-size helper so block formats are handled
as well. Preserve all application uploads, game data, filters, shader code and
depth behavior. Record this vendor correction in SDL's KINOKO_PATCHES.md.

`kinoko_gpu_transfer_contract` is an explicit manual GPU check, compiled by CI
and included in native packages but not registered for automatic CTest runs.
It uploads known pixels with padded rows, extra slice rows, a nonzero offset,
widths 1/16/32/64/96/160/544, and tightly packed defaults. It reads the GPU
texture back to check pixel values, without shaders or DAT. Compilation is
not execution; neither the agent nor CI runs this contract or the game.

Mac user verification remains required after delivery. The code defect and
static pixel reproduction are established; actual repaired Metal output has
not yet been observed on the user's Mac.

## Delivery evidence

Source: `92dfbd07f007f69170b7bb440e6f9760433a3f65`.
[CI 36525803562](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36525803562)
passed all six full-game/portable jobs. The new GPU contract compiled on all
three OSes, including both macOS architectures, but was not executed.

Local Windows x64 game and GPU transfer contract compiled in
`build-runs/modern-x64-macos-render-02/`. DAT validation and static D3D9 audit
passed. Shaders, platform window creation and shared renderer sources match
the pre-candidate `0c0f8064` versions: speculative depth/Retina edits are gone.

macOS universal delivery with user-owned DAT (local only):
`runtime-builds/modern-macos-render-02/6kinoko-modern-macos-universal-92dfbd07-with-data.tar.gz`.
SHA256: `36a85d9d58d4db182e9cfb18a5f144ec96f285141a3ebdcde809332cb32d6dc4`.
CI manifest hashes, DAT hashes and archive executable permissions were checked.
Download and staging evidence: `build-runs/macos-render-delivery-02/`.
Extract the entire new archive and launch `Launch.command`. Keep older packages
and saves. Target-machine visual/gameplay verification remains with the user.
