# Image-first extraction of the target UI font

The scope is the cartoon font represented by `gp_menu1.cv2`, `start_wd.cv2`
and `start_st1.cv2`, not every font in the game. No manually entered character
labels or cutting coordinates are used. No external fonts supply missing shapes.

## Separation of image extraction and recognition

`tools/font_extraction/export_unlabeled.py` decodes the original CV2 pixels,
including alpha, and saves unscaled RGBA PNGs. It preserves complete textures and
rows as well as proposed glyph crops. OCR success is not a prerequisite for
keeping a crop. Source selection inherits the previous automated style checks;
therefore the overall process is not independent of earlier OCR-based evidence.

Transparent backgrounds do not guarantee one isolated component per character.
Outlines and shadows can join letters; separate strokes and diacritics can form
one character. The script proposes cuts using alpha and bright-face projections,
keeps narrow-fragment unions as alternative hypotheses, and flags unusual widths.
Its exact-mask deduplication intentionally retains different sizes and treatments.
An uncut source always remains available. No candidate count is a character count.

`tools/font_extraction/label_candidates.py` reads these candidates into temporary
OCR inputs. Only these temporary inputs are enlarged. Two recognition models
must agree at confidence >= 0.95, with supporting prior source-text evidence,
before adding a Unicode mapping. The original PNGs and exported contours are
not changed by OCR. Repeated OCR agreement is not proof of correct recognition.
Existing experimental mappings are preserved, not re-certified by this pass.

The ordinary TTF still uses binary raster contours. It is **not** a lossless
container for antialiased image edges. The unscaled RGBA PNGs are the retained
pixel source for subsequent contour/edge work. OCR itself does not blur a font;
resampling, thresholding and vectorization may change its appearance.

## Local batch results (2026-09-28)

- `analysis/font-extraction-10/`: 33 accepted source textures, 193 preserved
  row variants, 1,204 candidate occurrences and 449 unique mask proposals.
- 56 other source textures are retained in quarantine because their font style
  was not accepted; they are not added to the candidate font.
- `analysis/font-extraction-11/`: 153 mapped characters plus space; one new
  mapping, U+5168, compared with the 152-character pass 07 font.
- Source PNG pixel round-trips, unchanged candidate-file hashes after OCR,
  exported cmap loading and actual TTF preview rendering were checked.
- No game build, game run, game modification or system font installation.
- **Incomplete**: ambiguous cutting, unresolved labels, source-font classification
  and coverage beyond the previous UI selection remain. The full resource scan
  remains in pass 04; it did not establish exhaustive glyph recovery.

The candidate archive TTF uses private-use codepoints for cut hypotheses. These
may include fragments, several joined characters or decorative shapes. It is a
review archive, not an expanded, correctly mapped text font. Use the pass 11
ordinary TTF for the currently mapped experimental subset.

All prior outputs are retained. Pass 08 includes the first failed font export;
pass 09 is the successful initial cut batch; pass 10 additionally preserves
alternative unions for disconnected strokes. Resources and font binaries stay
in ignored local analysis directories, outside the source commit.

## Reproduction

Run from the modern repository. Supply the original resource directory, the
existing entries inventory and pass 07 recognition/style evidence. Output paths
must not exist; use a new directory for every invocation.

```powershell
uv run --python 3.12 --with numpy --with opencv-python-headless --with pillow --with fonttools python tools/font_extraction/export_unlabeled.py --previous analysis/font-extraction-07 --entries docs/font-resource-investigation/entries.json --reference C:/WorkSpace/6kinoko --output analysis/font-extraction-NEW-cuts

uv run --python 3.12 --with rapidocr --with onnxruntime --with fonttools --with opencv-python-headless --with pillow python tools/font_extraction/label_candidates.py --candidates analysis/font-extraction-NEW-cuts --previous analysis/font-extraction-07 --output analysis/font-extraction-NEW-labels
```

`sources.json`, `rows.json`, `candidates.json` and `recognition.json` retain the
source paths, crop geometry, alternatives and mapping evidence. Full RGBA images
live under `sources/`, `rows/` and `candidates/`; `*-mask.png` files are working
masks, not substitutes for the original pixels. `validation.json` records the
scope and font checksum. Recognition cannot recover glyphs absent from the
original material or establish that two source textures share a font designer.
