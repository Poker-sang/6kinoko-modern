# x64 preparation build evidence

## ACT texture and host width checkpoint

Source: `4165595335a365feba9f7b98e2f1613c9d82a5b1`.

- `modern-resource-pointer-01`: full Win32 Release game and 69 contract
  executables compiled. Three original DAT were copied beside the EXE and
  checked by size and SHA256. Keep the adjacent shaders directory.
- Game: `runtime-builds/modern-resource-pointer-01/kinoko_modern_gpu.exe`.
- Static D3D9 audit passed: 105 compiler and 79 linker dependency logs,
  no violations. Report: `build-runs/modern-resource-pointer-01/d3d9-audit.json`.
- `modern-resource-pointer-x64-01`: ACT texture state, shared string boundary,
  actual `legacy_string.cpp`, and Squirrel object storage compiled separately
  for Windows x64. This does not compile or link the full x64 game.
- `modern-act-texture-01` and `modern-squirrel-width-01` failed during source
  migration. Their logs and any staged files are retained. The corrected builds
  used fresh names; no previous artifacts were overwritten.
- No game, CTest or contract executable was run. User reports the preceding
  `modern-font-01` build normal; this is user feedback, not agent validation.
- Exact x86 hashes and DAT checks: `build-runs/modern-resource-pointer-01/artifacts.json`.
  Final x64 compiler logs: `build-runs/modern-resource-pointer-x64-01/build.log`.


## Font runtime checkpoint

Source: `8181b134d3b5df545d80222740d95683f71893ae`.

- `modern-font-01`: full Win32 Release game and 67 contract executables compiled.
- Game: `runtime-builds/modern-font-01/kinoko_modern_gpu.exe`.
- Three DAT copied beside EXE and size/SHA256 checked; retain adjacent shaders.
- Static D3D9 audit passed: 101 compiler / 77 linker dependency logs, no violations.
- `modern-font-x64-01`: font runtime contract and actual GDI raster/upload object
  compiled with Windows x64. Static PE/COFF inspection confirms AMD64 for both.
  This is not a linked x64 game or an x64 rasterizer execution test.
- The isolated object compile emits C4297 for the existing C-linkage exception
  rethrow under default /EHsc. The full game retains its existing /EHsc- source
  override; the isolated object is compile evidence only and is not linked/run.
- No game, preview, CTest or contract executable was run. Runtime assertions are
  compiled, not passed. User reports modern-width-01 normal, separately from this batch.
- Exact artifacts: `font-01-artifacts.json`, `font-x64-01-artifacts.json`,
  `font-01-d3d9-audit.json`. All build/staging logs and products are retained.

# Runtime graphics width checkpoint

Both builds use source `686d54ba2b6bcfd5aeaf616ecd6685b42a920cbd`.

- `modern-width-01`: complete Win32 Release game and 66 active contract executables
  compiled successfully. No executable was run.
- Game: `runtime-builds/modern-width-01/kinoko_modern_gpu.exe`.
- Three original DAT copied beside the game, checked by size/SHA256. Keep adjacent
  `shaders/`. Exact hashes and shader hashes: `width-01-artifacts.json`.
- The complete dependency audit passed: 99 compiler and 76 linker input logs,
  with no D3D9/D3DX SDK inputs or game DLL/load-entry markers.
- `modern-width-x64-01`: only `kinoko_graphics_runtime_contract` compiled under
  `KINOKO_PLATFORM_ONLY=ON`, `-A x64`, Release. It includes runtime records,
  production listener registry and Windows public renderer/texture headers.
- Static PE header inspection confirms machine AMD64 (0x8664). Exact artifact
  and scope: `width-x64-01-artifacts.json`. This is not an x64 game build.
- Runtime order/context/removal assertions were compiled but not executed;
  compile-time layout/type assertions passed in both builds. No game, preview,
  CTest or contract executable was run.
- All configure/build/staging/static-audit logs and artifacts retained under the
  two build-runs/runtime-builds directories. The x64 executable is a contract,
  not the game handoff, and does not need DAT files.
- Width scan is review evidence, not a runtime test or complete x64 safety proof.
- User reports modern-cleanup-02 currently normal; not agent-run validation.
- Prior CI 36327876216 succeeded. The new runtime contract is included in the
  portable Windows x64/Linux/macOS CI matrix; current push CI is separate.
