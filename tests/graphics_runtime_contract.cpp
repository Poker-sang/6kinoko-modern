#include "kinoko/device_listeners.hpp"
#include <cstddef>
#include <type_traits>
#include <vector>
// Compile the actual public game declarations on every portable host.
#include "kinoko/renderer.h"
#include "kinoko/texture_store.h"

// LLP64 (Windows) and LP64 (Linux/macOS) must agree on command/status widths.
static_assert(sizeof(kinoko::graphics::Result) == 4);
static_assert(sizeof(kinoko::graphics::PixelRect) == 16);
static_assert(sizeof(kinoko::graphics::MappedPixels::Pitch) == 4);
static_assert(sizeof(kinoko::graphics::SurfaceDescription::Usage) == 4);
static_assert(kinoko::graphics::failed(kinoko::graphics::error_invalidcall));
static_assert(kinoko::graphics::succeeded(kinoko::graphics::ok));
static_assert(kinoko::graphics::succeeded(1));
static_assert(std::is_standard_layout_v<KinokoRenderer>);
static_assert(std::is_same_v<decltype(KinokoTextureSlot::texture), kinoko::graphics::Texture*>);
static_assert(std::is_same_v<decltype(KinokoDeviceListener::context), void*>);
static_assert(sizeof(KinokoTextureHandle) == 4); // Index format stays unchanged.
static_assert(sizeof(decltype(KinokoRenderer::device)) == sizeof(void*));
static_assert(sizeof(decltype(KinokoTextureSlot::texture)) == sizeof(void*));
static_assert(offsetof(KinokoRenderer, listener) > 0); // No object-prefix dispatch.
static_assert(offsetof(KinokoTextureSlot, width) == sizeof(void*));

namespace {
struct Owner { int id; std::vector<int>* calls; KinokoDeviceListener listener; };
void before(void* context) {
    auto& owner = *static_cast<Owner*>(context);
    owner.calls->push_back(owner.id);
}
void after(void* context) {
    auto& owner = *static_cast<Owner*>(context);
    owner.calls->push_back(owner.id + 10);
}
}
int main() {
    std::vector<int> calls;
    Owner first{1, &calls, {}}, second{2, &calls, {}};
    first.listener = {&first, before, after};
    second.listener = {&second, before, after};
    kinoko::graphics::DeviceListeners listeners;
    if (listeners.add(&first.listener) != 1 || listeners.add(&second.listener) != 1) return 1;
    if (listeners.add(&first.listener) != 0) return 2;
    listeners.notify(KINOKO_DEVICE_BEFORE_RESET);
    listeners.notify(KINOKO_DEVICE_AFTER_RESET);
    if (calls != std::vector<int>{1, 2, 11, 12}) return 3;
    calls.clear();
    listeners.remove(&first.listener);
    listeners.remove(&first.listener);
    listeners.notify(KINOKO_DEVICE_BEFORE_RESET);
    if (calls != std::vector<int>{2}) return 4;
    listeners.clear();
    listeners.notify(KINOKO_DEVICE_AFTER_RESET);
    if (calls != std::vector<int>{2} || first.id != 1) return 5;
    return 0;
}
