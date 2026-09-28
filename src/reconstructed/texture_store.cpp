#include "kinoko/runtime_util.hpp"
#include "graphics_texture_retirement.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/texture_store.h"
#include "kinoko/texture_image.h"
#include "kinoko/com_owner.hpp"

#include <array>
#include <string>

#include "kinoko/graphics_api.hpp"

extern "C" {
KinokoTextureSlot kinoko_texture_slots[KINOKO_TEXTURE_CAPACITY] = {};
}

namespace {
struct Ownership {
    std::string name;
    uint32_t references = 0;
};
std::array<Ownership, KINOKO_TEXTURE_CAPACITY> owners;

bool valid(int32_t handle) {
    return handle > 0 && handle < KINOKO_TEXTURE_CAPACITY &&
           owners[handle].references != 0;
}

// Original 406370 canonicalizes the resource name before the reference lookup.
std::string resource_key(const char *path) {
    std::string key(path);
    for (char &character : key)
        if (character == '\\') character = '/';
    kinoko::lower_asset_name(key.data(), static_cast<uint32_t>(key.size()));
    return key;
}
}

extern "C" int32_t kinoko_texture_register(
    kinoko::graphics::BaseTexture *texture, uint32_t width, uint32_t height) {
    if (!texture) return 0;
    for (int32_t handle = 1; handle < KINOKO_TEXTURE_CAPACITY; ++handle) {
        if (owners[handle].references) continue;
        owners[handle].references = 1;
        kinoko_texture_slots[handle] = {texture, width, height};
        return handle;
    }
    return 0;
}

extern "C" int32_t kinoko_texture_acquire(const char *path) {
    if (!path || !*path) return 0;
    try {
        auto key = resource_key(path);
        for (int32_t handle = 1; handle < KINOKO_TEXTURE_CAPACITY; ++handle) {
            auto &owner = owners[handle];
            if (owner.references && owner.name == key) {
                ++owner.references;
                return handle;
            }
        }
        kinoko::graphics::Texture *texture_value = nullptr;
        uint32_t width = 0, height = 0;
        if (kinoko_texture_load_image(path, &texture_value,
                           &width, &height) < 0 || !texture_value)
            return 0;
        // Loading transfers one COM reference. Registration adopts it only on
        // success; a full store leaves the temporary responsible for release.
        kinoko::ComOwner<kinoko::graphics::BaseTexture> texture(
            static_cast<kinoko::graphics::BaseTexture *>(texture_value));
        const auto handle = kinoko_texture_register(texture.get(), width, height);
        if (!handle) return 0;
        texture.detach();
        owners[handle].name.swap(key);
        return handle;
    } catch (...) {
        // Do not let C++ allocation exceptions cross the reconstructed C ABI.
        return 0;
    }
}

extern "C" int32_t kinoko_texture_release(int32_t handle) {
    if (!valid(handle)) return 0;
    auto &owner = owners[handle];
    if (--owner.references) return 1;

    auto &slot = kinoko_texture_slots[handle];
    // Adopt the store's final reference for this scope. The public slot is a
    // borrowed renderer view and remains intact throughout device unbinding.
    kinoko::render::GraphicsTextureRetirement texture(slot.texture);
    // Query actual bindings; the handle cache may differ after a failed bind.
    texture.unbind(kinoko_graphics.device);
    kinoko_texture_forget_bindings(handle); // original final-release cache invalidation
    texture.release(); // Preserve final Release before clearing the slot/name.
    slot = {};
    owner.name.clear();
    return 1;
}

extern "C" int32_t kinoko_texture_retain(int32_t handle) {
    if (!valid(handle)) return 0;
    ++owners[handle].references;
    return 1;
}
