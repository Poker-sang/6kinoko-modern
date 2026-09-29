#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace kinoko::input {
// Stable IDs. Append new actions; never renumber recorded frames.
enum Action { Left, Right, Up, Down, Jump, Attack, Run, Carry, Door, PipeUp,
    PipeDown, Confirm, MenuBack, Pause, UseItem, MenuLeft, MenuRight, MenuUp,
    MenuDown, Count };
inline constexpr const char* names[] = {"moveLeft", "moveRight", "moveUp", "moveDown",
    "jump", "attack", "run", "carry", "door", "pipeUp", "pipeDown", "confirm",
    "menuBack", "pause", "useItem", "menuLeft", "menuRight", "menuUp", "menuDown"};
struct Frame { int32_t held[Count]{}; uint8_t released[Count]{}; };
enum class Source { Legacy, Scan, Pad };
struct Binding { Source source; int index; };
using Bindings = std::array<std::vector<Binding>, Count>;
struct Sample {
    // Original signed axes and positive button counts, before script mutation.
    int32_t x{}, y{}, buttons[6]{};
    std::array<bool,256> keys{};
    std::array<bool,32> pad{};
};
Bindings classic_bindings();
// Transactional: errors leave destination untouched. key resolver returns -1 on error.
bool parse_bindings(std::string_view text, Bindings& destination, std::string& error,
                    int (*key_scan)(const std::string&));
void advance(const Bindings&, const Sample&, Frame&);
// Legacy defaults reuse original counters, including device handoffs.
int32_t legacy_count(Action, const Sample&);
}
