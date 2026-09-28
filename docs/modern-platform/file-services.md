# Portable file service (2026-09-28)

Next step after user acceptance of x64 savedata-01: remove Windows file handles
from the gameplay I/O boundary. DAT readers, loose-file readers/writers, savedata
and key/controller configuration now use KinokoFile and one common file service.
The public API and its RAII owner contain no Windows headers or types.

Windows backend keeps ANSI paths, read sharing, exclusive writes, short-read/EOF
semantics and unbuffered Win32 file operations. POSIX backend uses binary stdio,
64-bit fseeko/ftello and flushes writes. POSIX does not enforce Windows deny-share
locks. The wrapper separates signed 64-bit seek errors from valid 0xffffffff
positions. Runtime DAT records, keys, offsets and savedata words remain 32-bit.
Original package virtual seek/end quirks and decoding lengths are preserved;
the bounded loader API remains distinct. Path normalization/CP932 archive hashing
still live in the original resource lookup layer; this batch does not claim
archive filename lookup is completely portable.

Deleted unused file_io_legacy.h integer-address adapters and updated its three
include consumers. Removed obsolete x86-only reader layout assertions; native
member relationships remain checked. Active Win32 support and disk format fields
are retained. No DAT, original save or input configuration was changed.

Portable CI now compiles the service, its asset-free contract and the actual
file_io.cpp reader implementation on Windows/Linux/macOS. The contract covers
bytes, EOF, seek origins/errors, size preserving cursor, >4 GiB cursor positions
without huge writes, truncate and ownership. Archive and savedata contracts
continue covering production consumers. Contracts are compiled, not run.

Full Linux/macOS games remain blocked by host threading, fonts/IME, diagnostics,
Windows-specific archive case handling and other platform dependencies. This
is a migrated production service, not a complete cross-platform game release.

First builds files-01 exposed a script error-window dependency on indirectly
included windows.h. That host source now includes its own Windows dependency;
file-service and actual reader compilation succeeded. Failed builds are retained.

## Delivery

Source `47ec4e47067a075a91446ef2064f7afa0feb0abd`:
- Full x64 game: `runtime-builds/modern-x64-files-02/kinoko_modern_gpu.exe`.
- Win32: `runtime-builds/modern-files-02/kinoko_modern_gpu.exe`, 81 contracts compiled.
- x64 file-service, archive and savedata contracts compiled separately, along
  with the real reader implementation. None were executed.
- Three DAT staged and size/SHA256 verified on both builds; two sprite shaders
  beside each EXE. Both static D3D9 audits passed.
- Native file formats are unchanged. No original saves/configs copied or edited.
- Local WSL has no Linux installation. Non-Windows compilation awaits CI;
  no Linux/macOS full-game or runtime success is claimed.

User confirmed the preceding savedata-01 works. This files-02 batch is not yet
user-validated. All old successful/failed builds and logs are retained.
