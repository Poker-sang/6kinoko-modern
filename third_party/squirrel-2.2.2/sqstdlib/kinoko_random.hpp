#pragma once
#include <cstdint>

// Original Windows CRT rand/srand contract, including the default seed.
// DAT bytecode contains literal 32767 divisors; libc rand is not portable.
namespace kinoko {
class ScriptRandom {
public:
    static constexpr int maximum = 32767;
    void seed(std::uint32_t value) noexcept { state_ = value; }
    std::uint32_t state() const noexcept { return state_; }
    int next() noexcept {
        state_ = state_ * 214013u + 2531011u;
        return static_cast<int>((state_ >> 16) & maximum);
    }
private:
    std::uint32_t state_ = 1;
};
}
