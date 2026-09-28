# x64 preparation build evidence


## Full-game x64 compiler baseline and native method ABI

Source: `7bb7f95bd59cac9f982ee569bb6932468fecbec8`.

- `modern-x64-entry-03`: full Win32 Release no-trace game and 76
  contract executables compiled. Three DAT staged beside the EXE and verified by
  size/SHA256. Keep the adjacent shaders directory.
- Game: `runtime-builds/modern-x64-entry-03/kinoko_modern_gpu.exe` (still x86).
- EXE SHA256: `DD2874647B0C47A375C8EEF53490A3EB3F94AB65EBCF2BC291405DEB1745F73B`.
- D3D9 static audit passed: 115 compiler / 86 linker input logs, no violations.
- `modern-x64-entry-native-03`: method-call, ACT dispatch and serialized-hash
  contracts linked at x64. Actual script-call, crash-diagnostics and critical-section
  translation units compiled at x64. Static PE/COFF inspection identifies all
  three EXEs and three objects as AMD64.
- Five serialized-hash compile-time assertions passed against original type-ID,
  empty-range and high-bit/embedded-zero samples. Runtime contract assertions
  were only compiled, not executed.
- `modern-full-x64-03`: actual full-game target configured successfully but did
  NOT compile/link. Source layout/address guards remain enabled. The report has
  268 unique compiler diagnostics; these include cascades and are NOT that many
  independent tasks. Removing earlier service blockers exposed additional
  translation units, so diagnostic totals need not decrease monotonically.
- Remaining clusters: shared container/control storage, actor/camera/quad,
  ACT/document/layer/script records, map/input/collision records and integer-address
  helpers. Full report: `full-x64-03-blockers.json`.
- No game, preview, CTest or contract executable was run. User reports the previous
  `modern-string-native-01` normal (user feedback).
- Retained earlier attempts: entry-01/native-01 failed on a missing resource type
  declaration; full-x64-01 stopped when PowerShell treated a CMake warning as an
  exception. Entry-02/native-02 succeeded; full-x64-02 exposed guards and service
  errors. All old directories/logs/products remain intact.
- Evidence: `entry-03-artifacts.json`, `entry-03-d3d9-audit.json`,
  `entry-native-03-artifacts.json`, `full-x64-03-artifacts.json`;
  [full-game milestone and commands](full-game.md).


## Native string layout, glyph queue and shared atlas ownership

Source: `cde7471da6dda604c4d0e935a3aae2dafdb97963`.

- `modern-string-native-01`: complete Win32 Release no-trace game and 74
  contract executables compiled successfully. Three DAT copied beside the EXE
  and size/SHA256 checked; retain the adjacent shaders directory.
- Game: `runtime-builds/modern-string-native-01/kinoko_modern_gpu.exe`.
- EXE SHA256: `A39DA4D7D214C97342C490028D7664A8909A9DBFB4B66DF68EFA5AB21B176A5D`.
- Static D3D9 audit passed: 110 compiler / 84 linker dependency logs, no violations.
- `modern-string-native-x64-01`: actual native string ownership/clone code and
  texture/chip/mesh resource contracts compiled and linked. All four PE headers
  identify AMD64. These are independent contracts, not a full x64 game.
- Runtime assertions cover page stability, replication/source destruction, clone
  state, embedded NULs, release and constructor rollback; compiled, not executed.
- No game, preview, CTest or contract executable was run. User reports the previous
  `modern-mesh-native-01` normal; this is user feedback only.
- All artifacts/logs retained. Evidence: `string-native-01-artifacts.json`,
  `string-native-01-d3d9-audit.json`, `string-native-x64-01-artifacts.json`.
  See [scope and preserved behavior](string-native.md).

## Native mesh resource/render chain and portable decoder

Source: `3d4ecbec275239df9816a2df1c86a5382fcbc016`.

- `modern-mesh-native-01`: full Win32 Release no-trace game and 73 contract
  executables compiled successfully. Three original DAT were staged beside the
  game and checked by size/SHA256. Keep the adjacent shaders directory.
- Game: `runtime-builds/modern-mesh-native-01/kinoko_modern_gpu.exe`.
- EXE SHA256: `E6C1A2659C0F7474E26FE15E81A3C4D8D790858B1C3DEEAF29A1CBAA566FC3CF`.
- D3D9 static audit passed: 109 compiler and 83 linker dependency logs, no violations.
- `modern-mesh-native-x64-01`: actual native mesh resource ownership/string code
  and MSH/MAT decoder compiled/linked in independent contracts. Static PE headers
  identify both as AMD64. The GPU renderer/full game are not x64 build results.
- No game, preview, CTest or contract executable was run. Runtime assertions are
  compiled only. User reports `modern-act-chip-02` normal; that is user feedback
  on the preceding batch, not validation of this game version.
- All build/staging/audit logs and products are retained. Evidence:
  `mesh-native-01-artifacts.json`, `mesh-native-x64-01-artifacts.json`,
  `mesh-native-01-d3d9-audit.json`; [migration scope](mesh-native.md).


## Native chip resources and ACT method interfaces

Source: `caebb44f38c314564ff20c23f1fb804a554180ab`.

- `modern-act-chip-02`: full Win32 Release no-trace game and 71 contract
  executables compiled successfully. Three DAT copied beside the EXE and checked
  by size/SHA256. Keep the adjacent shaders directory.
- Game: `runtime-builds/modern-act-chip-02/kinoko_modern_gpu.exe`.
- EXE SHA256: `5A6623FA6F52AF9C7097BAA36334C6906EA04307C0D5F90247AB7357A7431248`.
- D3D9 static audit passed: 107 compiler and 81 linker dependency logs, no violations.
- `modern-act-chip-x64-02`: texture, chip and ACT method adapter contracts compiled
  and linked independently. Static PE headers identify all three as AMD64.
  This is not a full x64 game, and the adapters retain the explicit legacy ABI boundary.
- The first checkpoint `modern-act-chip-01` and `modern-act-chip-x64-01` also
  compiled successfully from `ca6ab41d7d7032f1b2a4bbf8d5329db932780689`; the final
  checkpoint includes the remaining key/timeline, 3D/map query and suspend/resume
  interface migration. Both checkpoints and their logs are retained.
- No game, preview, CTest or contract executable was run. All runtime assertions
  are compiled only. User reports the preceding `modern-act-native-02` normal;
  this is user validation, not an agent smoke test or validation of this build.
- Evidence: `act-chip-02-artifacts.json`, `act-chip-x64-02-artifacts.json`,
  `act-chip-02-d3d9-audit.json`; [scope](act-chip-methods.md).


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
