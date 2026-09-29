#pragma once
#include <filesystem>
#include <string>
namespace kinoko::runtime {
struct Options {
    std::filesystem::path save_dir, recording, playback, status, tas_dir, tas_output;
    std::string identity;
};
const Options& options();
bool parse_options(int argc, char** argv, std::string& error);
// Empty on an invalid path; never fall back to the ordinary save directory.
std::string save_path(const char* name);
}
