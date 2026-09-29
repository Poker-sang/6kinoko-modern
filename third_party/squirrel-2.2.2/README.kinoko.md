# Squirrel 2.2.2

The include/ and squirrel/ source directories, COPYRIGHT and HISTORY are
copied unchanged from the supplied ../squirrel-2.2.2/SQUIRREL2 tree.
See COPYRIGHT and the license notice in include/squirrel.h.

The build directly compiles all 12 core squirrel/*.cpp files into a static
library. ACT source-to-bytecode compilation uses an isolated compiler VM;
sq_compile/sq_compilebuffer/compilestring also use the source compiler on the
current game VM, preserving its constants, enums and error callback.
Verified object, stack, array, thread and other operations are accessed through
our separate src/squirrel/ bridge files. The reconstructed VM still executes
game scripts by default. The optional
Execute experiment remains gated by both KINOKO_ENABLE_SQUIRREL_CPP_VM and
KINOKO_SQUIRREL_CPP_EXECUTE=1; it is not a complete VM migration.

KINOKO_SQUIRREL2_ROOT may point to the supplied external tree for comparison.
ABI assertions require the original 32-bit object and VM layouts.

This README is project documentation, not an upstream source file. On
2026-09-17 all 39 vendored upstream files were compared byte-for-byte with both
the supplied source tree and squirrel_2.2.2_stable.tar.gz: no differences.
Per-file SHA256 results are in
analysis/function-inventory-20260917/squirrel-source-verification.json.

## Native-width / original bytecode boundary (2026-09-28)

sqobject.cpp retains native VM integers and pointer-sized hashing in memory but
reads/writes Kinoko CV4 closure tags, counts, integer literals, local-variable
indices, line information, default parameters and stack size as explicit 32-bit
little-endian wire values. Float, boolean and fixed opcode encodings are unchanged.
Out-of-range integer serialization and invalid negative/oversized section counts
fail with a VM error. RefTable exposes a read-only diagnostic reference-count query
so table diagnostics no longer traverse hardcoded x86 offsets.

## Native float canonicalization (2026-09-28)

sqobject.h clears the complete value union before assigning SQFloat, matching its
float constructor. On x64, a reused register otherwise retains upper bytes from a
pointer/integer; IsEqual and table lookup compare the complete raw union.
sqcompiler.cpp zero-initializes scalar constant objects for the same reason.
The original float bit equality (including signed zero) is not changed.

Replay format 1 observes the existing ScriptRandom state through kinoko_script_random_state(); its sequence and seeding behavior are unchanged.

## Script math precision (2026-09-29)

sqstdmath.cpp explicitly promotes arguments to double before C math calls and
narrows the result to SQFloat. This preserves the Windows C-header behavior and
prevents POSIX/C++ float overload selection from changing directed-rounding
results. For example, under FE_UPWARD, cos(binary32 0x3f5710c4) must narrow to
0x3f2ad9fc; the prior Linux float overload returned 0x3f2ad9fd, causing a replay
velocity difference at frame 3444. This is precision selection, not a replacement
random generator or a guarantee that every platform libm is bit-identical.
