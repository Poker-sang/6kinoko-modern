#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* Call on startup before launching workers. Uses the executable directory,
   never the reference asset directory. Returns 0 without changing cwd on error. */
int kinoko_use_executable_directory(void);
#ifdef __cplusplus
}
#endif
