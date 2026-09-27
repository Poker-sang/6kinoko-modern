#include "kinoko/act_texture_runtime.hpp"
#include <type_traits>

using kinoko::act::TextureRuntimeState;
static_assert(std::is_same_v<decltype(TextureRuntimeState::methods), const void*>);
static_assert(std::is_same_v<decltype(TextureRuntimeState::handle), std::int32_t>);
static_assert(std::is_trivially_copyable_v<TextureRuntimeState>);
static_assert(alignof(TextureRuntimeState) >= alignof(void*));

int main() {
    TextureRuntimeState source;
    source.handle = 19;
    source.source_width = 256.0f;
    const auto copy = source;
    return copy.handle == 19 && copy.source_width == 256.0f ? 0 : 1;
}
