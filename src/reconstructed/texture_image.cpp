#include "kinoko/texture_image.h"
#include "kinoko/bitmap.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "graphics_texture.hpp"
#include "kinoko/texture_pixels.hpp"
#include "kinoko/diagnostics.h"
#include <algorithm>
#include <cstring>
#include <type_traits>

extern "C" {
void kinoko_trace_i32(const char*,int32_t);
}

namespace {
int32_t diagnostic_address(const void* value) { return static_cast<int32_t>(reinterpret_cast<intptr_t>(value)); }
}

extern "C" kinoko::graphics::Result kinoko_texture_load_image(const char* path, kinoko::graphics::Texture** output,
    uint32_t* width, uint32_t* height) {
    if (!path || !output || !kinoko_graphics.device) return kinoko::graphics::error_invalidcall;
    const auto length = std::strlen(path);
    char lookup[MAX_PATH];
    if (length<3 || length+1>sizeof(lookup)) return kinoko::graphics::error_invalidcall;
    std::memcpy(lookup,path,length+1);
    std::memcpy(lookup+length-3,"cv2",3);
    kinoko::Bitmap owner;
    if (!kinoko_bitmap_load_cv2(owner.get(),lookup)) return kinoko::graphics::error_invalidcall;
    const auto& bitmap = *owner.get();
    const auto source_pitch = bitmap.bit_depth==16 ? (bitmap.row_width/2u)*4u :
        bitmap.bit_depth>=24 ? bitmap.row_width*4u : bitmap.row_width;
    if (!bitmap.width || !bitmap.height || bitmap.row_width<bitmap.width || !source_pitch ||
        uint64_t(source_pitch)*bitmap.height>256u*1024u*1024u)
        return kinoko::graphics::error_invalidcall;
    if (width) *width=bitmap.width;
    if (height) *height=bitmap.height;

    auto allocation_width=bitmap.width, allocation_height=bitmap.height;
    // 40E73D and 40E793: report source dimensions, then square allocation if
    // required by caps. The original graphics lock encloses texture creation.
    if (kinoko_graphics.capabilities.TextureCaps & kinoko::graphics::texture_caps_squareonly)
        allocation_width=allocation_height=(std::max)(allocation_width,allocation_height);
    const auto format = bitmap.bit_depth<=16 ? kinoko::render::PixelFormat::argb1555 : kinoko::render::PixelFormat::argb8888;
    kinoko::render::GraphicsTexture texture(kinoko_graphics.device);
    kinoko::graphics::Result result;
    {
        kinoko::graphics::Lock lock;
        result=texture.create({allocation_width,allocation_height,format});
    }
    kinoko_trace_hresult("texture:create-hr",result);
    kinoko_trace_i32("texture:create-object",diagnostic_address(texture.get()));
    if (kinoko::graphics::failed(result) || !texture) return result;
    if (!*reinterpret_cast<void***>(texture.get())) {
        kinoko_trace("texture:create-no-vtable");
        texture.detach(); // inherited invalid-interface boundary: Release is unavailable
        return kinoko::graphics::error_failure;
    }
    const kinoko::graphics::Result creation_result = result;
    kinoko::render::PixelMapping locked{};
    const kinoko::graphics::Result lock_result = texture.map(locked);
    kinoko_trace_hresult("texture:lock-hr",lock_result);
    kinoko_trace_i32("texture:lock-object",diagnostic_address(texture.get()));
    kinoko_trace_i32("texture:lock-bits",diagnostic_address(locked.pixels));
    kinoko_trace_i32("texture:lock-pitch",locked.pitch);
    // 40E7E0 tests exactly zero. A failed or nonzero-success lock skips upload,
    // but 40E705 still returns the creation status and hands off the texture.
    if (lock_result != kinoko::graphics::ok) {
        *output = texture.detach();
        return creation_result;
    }
    if (!locked.pixels || locked.pitch<=0) {
        kinoko_trace("texture:lock-invalid-surface");
        texture.unmap();
        return kinoko::graphics::error_failure;
    }
    if (!kinoko::render::copy_surface({bitmap.bit_depth,bitmap.width,bitmap.height,
            bitmap.encoded_size,bitmap.palette,bitmap.pixels},source_pitch,locked)) {
        texture.unmap();
        return kinoko::graphics::error_failure;
    }
    kinoko_trace_i32("texture:unlock-object",diagnostic_address(texture.get()));
    texture.unmap(); // 40E815 ignores this kinoko::graphics::Result.
    *output=texture.detach();
    return creation_result;
}
