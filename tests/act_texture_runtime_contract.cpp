#include "kinoko/act_texture_runtime.hpp"
#include "kinoko/act_texture_leases.hpp"
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
    int resource=0;
    kinoko::act::TextureCloneLeases leases;
    if (!leases.insert(&resource, 19) || leases.insert(&resource, 20)) return 1;
    if (leases.take(&resource) != 19 || leases.take(&resource).has_value()) return 2;
    return copy.handle == 19 && copy.source_width == 256.0f ? 0 : 3;
}
