#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoDirectorySearch KinokoDirectorySearch;
/* SDL-backed snapshot; names remain valid until close. Matching is uniformly
   case-insensitive. Paths/names use the existing native narrow-string boundary,
   converted to/from SDL UTF-8 through std::filesystem. *.* includes extensionless
   names; wildcards apply only to the final component. Dot entries are omitted. */
KinokoDirectorySearch* kinoko_directory_first(const char* pattern);
int kinoko_directory_next(KinokoDirectorySearch* search);
const char* kinoko_directory_name(const KinokoDirectorySearch* search);
int kinoko_directory_close(KinokoDirectorySearch* search);
#ifdef __cplusplus
}
#endif
