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
