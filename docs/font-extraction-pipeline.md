# Image-first extraction of the target UI font

The scope is the cartoon font represented by `gp_menu1.cv2`, `start_wd.cv2`
and `start_st1.cv2`, not every font in the game. No manually entered character
labels or cutting coordinates are used. No external fonts supply missing shapes.

## Latest expansion (2026-09-29)

The current local delivery is `analysis/font-extraction-25/`: **259 characters
plus space**, 106 more than the previous 153-character delivery. It contains the
TTF, a rendered preview, per-character original RGBA crops and working masks,
complete source textures, provenance, unresolved crops and coverage records.
This is still an incomplete experimental subset, not a claim that all original
glyphs have been recovered.

All 1,232 original CV2 entries were decoded with zero errors and passed through
full-image text detection in `analysis/font-resource-audit-15/`. The 447 positive
resources include false positives; they are not 447 font textures. Previously
missed illustrated help panels were processed using their dark text color.
Other improvements cover tiled credits, low-resolution boundary comparisons,
colored lettering, disconnected marks and recognition of individual cuts even
when sentence recognition fails. Score-popup textures found outside the UI were
also checked and failed the target-font comparison.

The release retains 52 source textures accepted by the automated family checks.
It retains 67 unresolved crop proposals and 10 unmapped character proposals from
past sentence OCR consensus. These are **not** counts of proven missing glyphs:
they include possible OCR errors, punctuation, fragments and repeated renditions.
Full-image detection can still miss text. Existing inherited mappings have not
all been re-certified, and automatic family similarity is not font provenance.

Mapping uses Japanese/English PP-OCRv4 and multilingual PP-OCRv5 mobile/server
recognizers, per-character context agreement, repeated-image evidence and
complete-sentence agreement. A single glyph need not be recognized in isolation
when two high-confidence full-sentence readings and safe cuts support its label.
Punctuation additionally uses overlapping text detections on the original image.
OCR reads temporary inputs and never redraws the exported source pixels.

Four newly added outline samples (N, h, れ, セ) were replaced automatically with
verified white glyph-face samples, avoiding hollow letter outlines from colored
credits tiles. This is source selection, not edge smoothing. Smoothing, spacing
and baseline refinement remain deferred.

`assemble_font_release.py` reopened the final cmap, checked it against every
provenance entry, rendered the actual TTF, and compared all 259 RGBA crops
pixel-for-pixel against decoded original assets. No game code changed, no game
build/play session ran, and no font was installed. All batches and logs remain.

New reusable tools:

- `audit_text_resources.py`: full-CV2 detection audit with pixel deduplication.
- `expand_text_regions.py`: detected text, row and tile extraction, family checks,
  multiple recognizers, context evidence and incremental font export.
- `assemble_font_release.py`: provenance, source-pixel verification, glyph-face
  selection, contextual punctuation recovery and a consolidated delivery.

Example commands (fresh output paths are required):

```powershell
uv run --python 3.12 --with rapidocr --with onnxruntime --with fonttools --with opencv-python-headless --with pillow python tools/font_extraction/audit_text_resources.py --output analysis/font-resource-audit-NEW

uv run --python 3.12 --with rapidocr --with onnxruntime --with fonttools --with opencv-python-headless --with pillow python tools/font_extraction/expand_text_regions.py --server-recognizer --base-font analysis/font-extraction-11/kinoko-raster-experimental.ttf --output analysis/font-extraction-NEW

uv run --python 3.12 --with numpy --with opencv-python-headless --with pillow --with fonttools python tools/font_extraction/assemble_font_release.py --latest analysis/font-extraction-22 --output analysis/font-release-NEW
```

The consolidation command intentionally consumes the retained local pass chain
16–22, baseline passes 07/10/11 and audit 15. Those resource-derived artifacts are
local inputs, not files published with the repository. Raw assets and fonts have
not been pushed; the scripts and this handoff are the source-code backup.

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
