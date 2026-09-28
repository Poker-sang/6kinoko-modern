# x64 startup crash: native Sqrat integer conversion

## User report and diagnosis

The user reported `modern-full-x64-24` stayed black, then `modern-x64-callback-01`
exited without a picture. These are user observations, not agent execution.
The user launched callback-01 with existing diagnostics enabled. The resulting
log and two full-memory dumps remain beside that EXE; they were not deleted.
The agent did not launch the game or execute tests.

The recorded exception was C0000005, a read from zero at RVA `0x9e6d6` in source
`c9eee468`. IDA analysis of this exact executable identifies
`kinoko::script::upstream::sqrat_integer_argument`: after accepting OT_INTEGER or
OT_FLOAT, it calls `sq_getinstanceup`, then dereferences its null output.
The trace had progressed through compiled scripts and Fader2 ACT initialization;
this does not establish complete bytecode compatibility or successful rendering.
See `x64-integer-01-crash-analysis.json` and `x64-callback-01-user-crash.log`.

## Fix

Sqrat 0.8.1 guarded its 64-bit scalar specializations with `defined(__int64)`.
MSVC treats __int64 as a keyword, not a macro, so Windows x64 SQInteger selected
the generic class-instance template. The fix explicitly specializes standard
signed/unsigned long long, including the existing const/reference variants.
MSVC __int64 and long long are the same type. This repairs both argument reads
and integer pushes without narrowing native VM values or changing disk formats.

The production adapter now statically checks that SQInteger selects scalar
conversion. The upstream contract covers signed and wide integers, push type,
float conversion, invalid arguments and const/reference variants on root/child VMs.
Vendor provenance hashes were updated; all 322 upstream members/patches verified.

The preceding callback fix remains: successful updates return call status rather
than a truncated VM pointer; collision temporary storage/payload uses native
width; ACT registration and string-property return counts no longer depend on
heap-address sign bits. It fixed real defects but was not proven to explain the
original black screen. Its deterministic callback contract is retained.

## Delivery

Final source: `7f1044f0614b5b40ff5c03643760d23aa3de4968`.

- x64: `runtime-builds/modern-x64-integer-01/kinoko_modern_gpu.exe`, full Release no-trace game compiled/linked; statically AMD64 / PE32+.
- SHA256: `12a7509516cd212875b5124e1ff63f3490257da54982eeb2833c4fe22bd8e0a2`.
- Three DAT copied beside EXE and size/SHA256 verified; two DXBC shaders staged.
- Upstream bindings and callback-status contracts compiled/linked separately for x64, not executed.
- Win32: `runtime-builds/modern-integer-width-01/kinoko_modern_gpu.exe`; full build and 79 contracts compiled, DAT verified.
- Both D3D9 static audits passed. x64 compiler/linker summarizer reports zero errors.
- No gameplay/contract execution by the agent. The captured crash mechanism is fixed in source; final runtime behavior still awaits user feedback.

`run-with-diagnostics.cmd` beside the x64 EXE enables existing trace, first-chance,
script-failure and crash-dump capture for that child process only. Ordinary EXE
launch remains no-trace by default. This launcher was prepared, not executed by
the agent. Trace appends; fault/dump names include timestamp and PID. Keep the
whole runtime directory including resources. All previous build trees, binaries,
logs and dumps remain available; no font-investigation files were changed.

Evidence files use `x64-integer-01-*`, `integer-width-01-*`, `x64-callback-01-*`
and `callback-width-01-*` prefixes in this directory. Large user dumps stay local.
