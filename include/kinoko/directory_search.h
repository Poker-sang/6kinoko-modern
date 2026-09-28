#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoDirectorySearch KinokoDirectorySearch;
/* A search owns its current name until next/close. Windows preserves ANSI
   FindFirstFile behavior; POSIX uses UTF-8, native case sensitivity and order.
   Patterns accept slash/backslash separators; *.* also matches extensionless
   names. Wildcards are supported in the final component, not parent paths. */
KinokoDirectorySearch* kinoko_directory_first(const char* pattern);
int kinoko_directory_next(KinokoDirectorySearch* search);
const char* kinoko_directory_name(const KinokoDirectorySearch* search);
int kinoko_directory_close(KinokoDirectorySearch* search);
#ifdef __cplusplus
}
#endif
