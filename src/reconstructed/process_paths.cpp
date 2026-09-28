#include "kinoko/base_utilities.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>

extern "C" char* kinoko_game_window_slot; // Remaining game-window ABI consumers borrow this HWND.
namespace {
KinokoProcessContext context{};
inline char*& game_window_handle = kinoko_game_window_slot;
}

extern "C" const KinokoProcessContext* kinoko_process_context() { return &context; }
extern "C" int32_t kinoko_process_initialize(HINSTANCE instance, HWND window) {
    context.instance = instance; // Original 51B060, previously discarded.
    context.window = window;
    game_window_handle = reinterpret_cast<char*>(window);
    // Executable-directory selection is performed once before host startup.
    return 1;
}
extern "C" int32_t kinoko_path_split(const char* path, char* directory, char* file) {
    if (!path || !directory) return EINVAL;
    char drive[MAX_PATH]{};
    char folder[MAX_PATH]{};
    char name[MAX_PATH]{};
    char extension[MAX_PATH]{};
    auto error = _splitpath_s(path, drive, sizeof(drive), folder, sizeof(folder),
        file ? name : nullptr, file ? sizeof(name) : 0,
        file ? extension : nullptr, file ? sizeof(extension) : 0);
    if (error) { directory[0] = 0; if (file) file[0] = 0; return error; }
    if (file) {
        error = strcpy_s(file, MAX_PATH, name);
        if (!error) error = strcat_s(file, MAX_PATH, extension);
        if (error) { directory[0] = 0; return error; }
    }
    error = strcpy_s(directory, MAX_PATH, drive);
    return error ? error : strcat_s(directory, MAX_PATH, folder);
}
