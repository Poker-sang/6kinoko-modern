#pragma once
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

// Atomic directory creation avoids collisions; fixtures are deliberately kept.
inline std::filesystem::path retained_fixture(const char* prefix) {
    const auto base=std::filesystem::current_path()/"Testing"/"fixtures";
    std::filesystem::create_directories(base);
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned i=0;i<1000;++i) {
        auto path=base /
            (std::string(prefix)+"-"+std::to_string(stamp)+"-"+std::to_string(i));
        if(std::filesystem::create_directory(path)) return path;
    }
    throw std::runtime_error("Cannot allocate retained fixture directory");
}
