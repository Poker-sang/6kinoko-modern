# x64 preparation build evidence

## Native ACT texture resource chain

Source: `57356a0c83b7031aef720373961dbd9c72c8f90a`.

- `modern-act-native-02`: full Win32 Release no-trace game and 69 contract
  executables compiled. Three DAT staged beside the game and checked by size/SHA256.
- Game: `runtime-builds/modern-act-native-02/kinoko_modern_gpu.exe`.
  Preserve the adjacent shaders and DAT files.
- EXE SHA256: `6E0C56544544E2275B09061CD869BBC5834F01742A22A7AE8F9E0C0EB5E80D29`.
- D3D9 static audit passed: 105 compiler and 79 linker dependency logs, no violations.
- `modern-act-native-x64-02`: native texture object, bridge and actual string
  implementation linked as an isolated contract; PE machine is AMD64 (0x8664).
  This is not the full x64 game. Runtime assertions were not executed.
- First attempts `modern-act-native-01` and `modern-act-native-x64-01` failed
  linking the new contract because the explicit-length string module still
  contained the Windows missing-length adapter entry. The adapter entry moved
  beside its existing Windows scanner; corrected builds used fresh directories.
  All failed/successful artifacts and logs are retained.
- No game, preview, CTest or contract executable was run. The user confirmed
  the earlier window-move version only; no gameplay validation is claimed here.
- Evidence: `act-native-02-artifacts.json`, `act-native-x64-02-artifacts.json`,
  `act-native-02-d3d9-audit.json`. Scope: [native texture migration](act-texture-native.md).


## ACT texture clone ownership

Source: `6a2aa8831d8c1e9d03855c136865fbc4d41c14f7`.

- `modern-act-lease-01`: full Win32 Release game and 69 contract executables
  compiled. Three original DAT were staged beside the EXE and checked by size
  and SHA256; keep the adjacent shaders directory.
- Game: `runtime-builds/modern-act-lease-01/kinoko_modern_gpu.exe`.
- EXE SHA256: `144DA41BCBDAB854A39CA4510859D180538E901AE6F0B69B0C3FFB4598A8CFAE`.
- Static D3D9 audit passed: 105 compiler and 79 linker dependency logs, no
  violations. `modern-act-lease-x64-01` compiled the native texture ownership
  contract for Windows x64. Neither result is an x64 game build.
- Build/DAT/audit logs and hashes remain under the two unique build directories.
  No game or contract executable was run. User reports `modern-window-move-01`
  normal; this is user validation of the preceding build only.


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
