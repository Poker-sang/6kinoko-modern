#pragma once
#include <array>
#include <cstdint>
#include <memory>
struct SDL_Window;
namespace kinoko::platform {
// Legacy scan identifiers are data-format IDs, not OS-specific keycodes.
struct Controller {
    std::array<std::int32_t, 6> axes{};
    std::array<std::int32_t, 2> sliders{};
    std::array<std::uint32_t, 4> pov{0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu};
    std::array<std::uint8_t, 32> buttons{};
};
struct Input {
    std::array<std::uint8_t, 256> keys{};
    std::array<Controller, 16> controllers{};
    int controller_count{};
    std::int32_t mouse_x{}, mouse_y{}, mouse_wheel{};
    std::array<std::uint8_t, 8> mouse_buttons{};
};
int legacy_scan(int sdl_scan) noexcept;
std::int32_t legacy_axis(std::int16_t value) noexcept;
class Platform {
public:
    Platform();
    ~Platform();
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    // Window lifecycle and pump are main-thread only; consume_input is a
    // synchronized handoff to the simulation thread. Relative motion is consumed once.
    bool open(const char* title, int width, int height, bool hidden = false);
    bool pump();
    Input consume_input();
    SDL_Window* window() const noexcept;
    void close();
private:
    struct State;
    std::unique_ptr<State> state_;
};
Platform& host();
}
