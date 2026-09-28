# x64 preparation build evidence

## Native input/audio/application checkpoint

- Source: `f551e898c4f77ed9e4432a9663907b6a452569f7`.
- `modern-input-audio-02`: complete Win32 Release no-trace game and 77 contracts
  compiled. Three DAT copied beside the EXE and size/SHA256 verified.
- EXE: `runtime-builds/modern-input-audio-02/kinoko_modern_gpu.exe`.
- EXE SHA256: `8660EAC30761B88EDED1AFFEAD1C138DB6B32FB600EF6B68D84F3896F409DF76`.
- D3D9 static audit passed: 121 compiler / 87 linker logs, zero violations.
- `modern-input-audio-x64-04`: 12 production translation units + one signature/
  layout assertion unit compiled. All 13 objects statically verified AMD64.
  Covers audio runtime, physical input, aggregation/copy/frame/keys/configuration,
  script registration, application/game loop, scene queue and IME.
- `modern-full-x64-17`: full game remains blocked before linking; 62 unique
  diagnostics (previous 114). Audio/input/application layout groups cleared.
  Counts contain cascades, not independent tasks or a completion percentage.
  Remaining groups: file/bitmap services, actor method/diagnostic boundaries,
  remaining host/VM bridges. Original bytecode/save compatibility is still pending.
- Retained attempts: independent x64-01 (missing pair header), x64-02 (pair API
  argument mismatch), x64-03 (successful intermediate compile), Win32-01 (old
  scene fixture call signatures), full-x64-16 (intermediate probe, 62 diagnostics).
  Final registration diagnostics use ObjectView for the wrapped class value.
- No game, CTest or contract executable executed. Acceptance of modern-act-map-02
  is user feedback, not agent runtime validation.
- Evidence: `input-audio-02-artifacts.json`, `input-audio-02-d3d9-audit.json`,
  `input-audio-x64-04-artifacts.json`, `full-x64-17-artifacts.json`,
  `full-x64-17-blockers.json`. [Scope](input-audio-application-native.md).

## Native ACT publication/map/collision checkpoint

- Final build source: `28eb3e9c3110a2fc12218b7fc7f682afef9e8e6d`.
- `modern-act-map-02`: complete Win32 Release no-trace game plus 77 contracts
  compiled. Three DAT copied beside the EXE and size/SHA256 verified.
- EXE: `runtime-builds/modern-act-map-02/kinoko_modern_gpu.exe`.
- EXE SHA256: `75C0988C23B88826526262F49645C56FCBA00B750BA4093DB5ABB8CA67612620`.
- D3D9 static audit passed: 120 compiler / 87 linker logs, no violations.
- `modern-act-map-x64-06`: 47 production translation units + one signature/layout
  assertion unit compiled; all 48 COFF objects statically verified AMD64. Includes
  all reconstructed act_*.cpp, ACT script publication, map/collision, actor motion,
  text render consumers, camera/map registration and relevant VM property bridges.
- `modern-full-x64-15`: real full game remains blocked before linking with source
  guards enabled; 114 unique diagnostics (previous 125). Map/ACT/collision groups
  cleared while 28 audio-layout diagnostics became visible. Counts include cascades
  and are neither remaining task counts nor a completion percentage. Remaining
  work is audio/input/application/bitmap/file services, actor diagnostics/method
  boundary and remaining VM bridges. Bytecode/save/runtime compatibility is pending.
- No game, CTest or contract executable was run. User's prior acceptance remains
  user feedback, not agent runtime validation.
- Retained attempts: x64-01 (map pointer type / legacy header), x64-02 (remaining
  VM array), x64-03 (template allocation alias), x64-04 (subset compile success),
  x64-05 (expanded source set success), Win32-01 (one fixture missing its explicit
  legacy-memory include), full-x64-14 (intermediate full probe). No artifacts deleted.
- Evidence: `act-map-02-artifacts.json`, `act-map-02-d3d9-audit.json`,
  `act-map-x64-06-artifacts.json`, `full-x64-15-artifacts.json`,
  `full-x64-15-blockers.json`. [Scope and remaining work](act-map-publication-native.md).

## Native ACT rendering/lifetime priority checkpoint

- Build source: `bed6033e0d13ff16ce05bc046dae3ee57b132afa`.
- `modern-act-render-01`: full Win32 Release no-trace game and 77 contracts
  compiled; three DAT copied beside the EXE and size/SHA256 verified.
- EXE: `runtime-builds/modern-act-render-01/kinoko_modern_gpu.exe`.
- EXE SHA256: `B4DC78EA21F33A6E09E7D6B8972B5701C5FC48D2211BB208A1E9BDC675711FED`.
- D3D9 static audit passed (119 compile / 87 link dependency logs).
- `modern-act-render-x64-02`, source `1dcadbc3`: 11 real production translation
  units compiled; all 11 COFF objects statically identified as AMD64.
- `modern-full-x64-13`: full game remains blocked before linking, with 125 unique
  compiler diagnostics (previous checkpoint 133). Cascades are included; these
  are not independent tasks or a completion percentage. Architecture guards
  remain enabled. ACT publication/map/mesh-manager paths are not closed.
- All attempts retained, including the declaration failure in x64-01. No game,
  CTest or contract executable was run; no new user runtime feedback is assumed.
- Evidence: `act-render-01-artifacts.json`, `act-render-01-d3d9-audit.json`,
  `act-render-x64-02-artifacts.json`, `full-x64-13-artifacts.json` and
  `full-x64-13-blockers.json`. See [scope and remaining work](act-render-native.md).

## Native script bindings and ACT document/layer ownership

Build source: `f32f24c6108ee1d42be9477d1432f4457ba9a197` (documentation follows
implementation commit `91ad7c97bd6279db92cc349378784286d2ce092e`).

- `modern-binding-act-08`: complete Win32 Release no-trace game and 77 contract
  executables compiled. Three DAT staged beside the EXE and size/SHA256 checked.
- Game: `runtime-builds/modern-binding-act-08/kinoko_modern_gpu.exe`.
  Static PE inspection confirms I386. Keep adjacent shaders and DAT files.
- EXE SHA256: `4B7E2DD52121F546354995874A97826263B92C1DBB0542EE81AFFB007B92E3B6`.
- D3D9 static audit passed: 118 compiler / 87 linker dependency logs, no violations.
- `modern-binding-act-x64-07`, source `91ad7c97bd6279db92cc349378784286d2ce092e`:
  30 actual production translation units plus one layout/signature assertion
  translation unit compiled. Static COFF inspection confirms all 31 objects are
  AMD64. This target is an object compilation, not a linked x64 game.
- Actual source coverage: common SqPlus class/property/argument/method adapters,
  global registration and game closure adapters, source SqPlus metadata/type
  lookup, Sqrat/host object bridges, VM bootstrap/value/GC, ACT document/layer
  construction/clone/lifetime, script IO/lifetime, association, frame update and
  runtime lifetime. Native SQFUNCTION signatures and storage ties are checked.
- `modern-full-x64-12`: actual complete game graph configured but remains blocked
  before linking. Guards enabled. 133 unique diagnostics versus the preceding
  batch's 438; counts include cascades and are not independent tasks or a progress
  percentage. Main remaining clusters are map/collision/input/application,
  bitmap/file services, ACT layout/draw and remaining VM bridge consumers.
- User reports modern-actor-native-03 normal. No game, preview, CTest or contract
  executable was executed by the agent in this batch. Runtime assertions are
  compiled only; x64 bytecode/save-format compatibility is still pending.
- Retained attempts: binding-act-01/x64-01 (header boundary/scalar issues),
  binding-act-02/x64-02 (standard VM include order), binding-act-03 (old fixture
  address helpers), binding-act-04/05 (successful intermediate Win32/x64 builds),
  binding-act-06 (successful Win32; x64-06 exposed an unnecessary input-layout
  include). Earlier full-x64-06..10 probes/logs are retained. The staged-07 and
  full-x64-11 commands stopped before building because documentation needed a
  commit; final attempts used fresh names. No artifacts were deleted/overwritten.
- Evidence: `binding-act-08-artifacts.json`, `binding-act-08-d3d9-audit.json`,
  `binding-act-x64-07-artifacts.json`, `full-x64-12-artifacts.json`,
  `full-x64-12-blockers.json`. [Detailed scope](binding-act-native.md).



## Native actor/camera ownership and foundational containers

Source: `91fd8d7c4325b2fa4c0b3d3149ac0e99f6d230cb`.

- `modern-actor-native-03`: complete Win32 Release no-trace game and 77 contract
  executables compiled. Three DAT copied beside the EXE and size/SHA256 verified.
- Game: `runtime-builds/modern-actor-native-03/kinoko_modern_gpu.exe` (x86).
  Keep the adjacent shaders and DAT files.
- EXE SHA256: `5A60E01A3C3CC9314BE8D94F86C3E2799FDDFA7B02286282B04EBD824ADD30D7`.
- Static D3D9 audit passed: 117 compiler / 87 linker dependency logs, no violations.
- `modern-actor-native-x64-02`, source `bdc03ba995117eb18e695ff6bf1b437fdc6e5bb6`:
  17 actual actor/camera/container/lifecycle sources compiled; native ownership
  contract linked. Static inspection confirms AMD64 for all 17 objects and the
  contract EXE. The subsequent source fix affects the full camera binding, outside
  this isolated target. Neither target is a complete x64 game.
- `modern-full-x64-05`: actual game graph configured but compilation/linking
  remains blocked. Guards intact; 438 unique diagnostics, including cascades.
  Actor/camera native record assertions are no longer the blocking cluster.
  Remaining major groups: Squirrel object/registration/property bridges, ACT
  document/layer/resource records, map/collision/input/application records.
  More source files are reached than in the prior probe; totals are not task counts.
- Retained failed attempts: native-01/x64-01 (missing bootstrap declaration and
  obsolete animation-storage include), native-02 (camera binding call-site typo),
  full-x64-04 (pre-fix full compiler probe). Nothing was deleted or overwritten.
- No game, preview, CTest or contract executable was run. Runtime assertions are
  compiled only. User reports modern-x64-entry-03 normal, not agent validation.
- Evidence: `actor-native-03-artifacts.json`, `actor-native-03-d3d9-audit.json`,
  `actor-native-x64-02-artifacts.json`, `full-x64-05-artifacts.json`,
  `full-x64-05-blockers.json`. See [migration scope](full-game.md).




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
