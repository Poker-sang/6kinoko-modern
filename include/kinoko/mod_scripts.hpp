#pragma once
#include <string>
struct SQVM;
namespace kinoko::mods {
// Called once after the original boot script, before the first gameplay frame.
bool run_scripts(SQVM* vm, std::string& error);
}
