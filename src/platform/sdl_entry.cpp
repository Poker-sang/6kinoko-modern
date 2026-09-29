#include "kinoko/application.h"
#include "kinoko/diagnostics.h"
#include "kinoko/runtime_options.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <exception>
#include <stdexcept>
int main(int argc, char** argv) {
    kinoko_diagnostics_initialize();
    int result=1;
    try {
        std::string error;
        if(!kinoko::runtime::parse_options(argc,argv,error))throw std::runtime_error(error);
        result=kinoko_application_run(1);
    }
    catch(const std::exception& e) { SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Startup error",e.what(),nullptr); }
    kinoko_diagnostics_shutdown();
    return result;
}
