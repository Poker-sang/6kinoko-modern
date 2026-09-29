#pragma once
#include "kinoko/input_actions.hpp"
struct KinokoInputManager;
bool kinoko_replay_start();
void kinoko_replay_finish();
bool kinoko_replay_begin_frame();
void kinoko_replay_end_frame();
void kinoko_replay_input(KinokoInputManager*,kinoko::input::Frame&);
bool kinoko_replay_allow_rebind();
// Optional UI/reporting hook; null restores the game's SDL error dialog.
void kinoko_replay_set_error_handler(void (*handler)(const char*));
