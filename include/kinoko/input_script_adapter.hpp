#pragma once
#include <squirrel.h>
#include <string>
// Adapts the closure at stack top atomically. Original DAT is never modified.
bool kinoko_adapt_input_script(HSQUIRRELVM vm, std::string& error);
