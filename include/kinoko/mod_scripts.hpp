#pragma once
#include <string>
#include <vector>
struct SQVM;
struct SDL_Window;
namespace kinoko::mods {
// Called once after the original boot script, before the first gameplay frame.
bool run_scripts(SQVM* vm, std::string& error);
struct StageChoice {std::string id,name;};
bool list_stages(SQVM* vm,std::vector<StageChoice>& stages,std::string& error);
bool launch_stage(SQVM* vm,const std::string& id,std::string& error);
bool select_startup_stage(SQVM* vm,SDL_Window* window,const char* selection,std::string& error);
}
