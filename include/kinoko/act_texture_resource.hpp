#pragma once

#include "kinoko/legacy_string.hpp"
#include "kinoko/resource_allocation.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>

namespace kinoko::act {

// Native runtime object. Only the method/ID/name prefix is still shared with
// the x86 ACT publication ABI. File properties are serialized individually.
// Explicit cleanup remains in the ACT deleting-destructor adapter; allocation
// uses native aligned allocation metadata for scalar and array lifetimes.
struct TextureResource final {
    const void* methods = nullptr;
    std::int32_t id = -1;
    legacy::StringRecord name{{}, 0, 15};
    legacy::StringRecord texture_name{{}, 0, 15};
    std::int32_t texture = 0;
    std::int32_t width = 256, height = 256;
    float source_x = 0, source_y = 0, source_width = 0, source_height = 0;
    std::uint8_t borrows_texture = 0;
    std::uint8_t auto_size = 1;
    // Store the retained reference separately: scripts/create-target may change
    // the visible handle without changing which reference the clone acquired.
    std::int32_t retained_texture = 0;

    TextureResource() = default;
    TextureResource(const TextureResource&) = delete;
    TextureResource& operator=(const TextureResource&) = delete;

    std::int32_t take_retained_texture() noexcept {
        return std::exchange(retained_texture, 0);
    }
    // Explicit Unload and destruction share this one-time ownership transition.
    std::int32_t take_texture_reference() noexcept {
        const auto retained = take_retained_texture();
        const auto visible = std::exchange(texture, 0);
        return retained ? retained : (borrows_texture ? 0 : visible);
    }
    bool copy_names(const TextureResource& source) {
        const legacy::StringView source_name(const_cast<legacy::StringRecord*>(&source.name));
        const legacy::StringView source_texture(const_cast<legacy::StringRecord*>(&source.texture_name));
        legacy::StringView(&name).assign(source_name.data(), source_name.length());
        legacy::StringView(&texture_name).assign(source_texture.data(), source_texture.length());
        return name.length == source.name.length && texture_name.length == source.texture_name.length;
    }
    void clear_names() noexcept {
        legacy::StringView(&texture_name).destroy();
        legacy::StringView(&name).destroy();
    }
    void copy_image_properties(const TextureResource& source) noexcept {
        width = source.width; height = source.height;
        source_x = source.source_x; source_y = source.source_y;
        source_width = source.source_width; source_height = source.source_height;
        auto_size = source.auto_size;
    }
};
static_assert(std::is_standard_layout_v<TextureResource>);
static_assert(std::is_trivially_destructible_v<TextureResource>);
#if INTPTR_MAX == INT32_MAX
static_assert(offsetof(TextureResource, id) == 4);
static_assert(offsetof(TextureResource, name) == 8);
#endif
inline TextureResource* create_texture_resource(const void* methods, std::size_t count = 1) noexcept {
    return allocate_resources<TextureResource>(methods, count);
}
inline TextureResource& texture_resource(void* resource) noexcept {
    return *static_cast<TextureResource*>(resource);
}
inline const TextureResource& texture_resource(const void* resource) noexcept {
    return *static_cast<const TextureResource*>(resource);
}

} // namespace kinoko::act
