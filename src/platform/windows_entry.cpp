#include "kinoko/diagnostics.h"

#include "kinoko/application.h"
#include "kinoko/runtime_options.hpp"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static int startup_show_command=1;
static int SDLCALL application_main(int argc,char** argv) {
    std::string error;
    if(!kinoko::runtime::parse_options(argc,argv,error)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Startup arguments",error.c_str(),nullptr);return 1;
    }
    return kinoko_application_run(startup_show_command);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    startup_show_command=show_command;
    kinoko_diagnostics_initialize();
    int result;
    __try {
        result = SDL_RunApp(0,nullptr,application_main,nullptr);
    } __except (kinoko_report_exception(GetExceptionInformation())) {
        result = -1;
    }
    kinoko_diagnostics_shutdown();
    return result;
}
