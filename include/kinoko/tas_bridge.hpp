#pragma once
#include <cstdint>
#include <string>
struct SDL_Window;
namespace kinoko::tas {
bool enabled();
bool embedded();
void pump_window(SDL_Window* window);
void start();
uint64_t frame_interval_ns();
void pace_frame();
void shutdown();
// Worker-side frame boundary; waits for an acknowledged command while paused.
bool boundary(uint64_t completed,uint64_t total,bool live);
bool take_control();
uint32_t input_mask();
void completed(uint64_t count);
uint64_t frame_number();
void failure(const std::string& message);
void finish();
// Main render thread: explicit frame identity travels with the queued render packet.
void image(uint64_t count,uint32_t width,uint32_t height,const void* rgba);
}
