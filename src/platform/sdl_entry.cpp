#include "kinoko/application.h"
#include "kinoko/diagnostics.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <exception>
int main(int, char**) {
    kinoko_diagnostics_initialize();
    int result=1;
    try { result=kinoko_application_run(1); }
    catch(const std::exception& e) { SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Startup error",e.what(),nullptr); }
    kinoko_diagnostics_shutdown();
    return result;
}
