#include "kinoko/runtime_util.hpp"
#include "kinoko/act_method_dispatch.hpp"
#include "graphics_blend_sink.hpp"
#include "graphics_draw_state.hpp"
#include "kinoko/legacy_string.h"
#include "kinoko/graphics_device.h"
#include "kinoko/string_layout.h"
#include "kinoko/act_frame.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_texture_bridge.hpp"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/method_entry.hpp"
#include "kinoko/memory_access.hpp"
#include "kinoko/script_diagnostics.hpp"
#include "kinoko/act_layout_records.hpp"
#include "kinoko/texture_store.h"
#include "kinoko/render_target.h"
#include "kinoko/runtime_sync.hpp"
#include "kinoko/graphics_api.hpp"

extern "C" {
void kinoko_trace_i32(const char*, int32_t);
void kinoko_trace_squirrel_name(const char*, int32_t);

}

namespace {
using namespace kinoko::act;
using namespace kinoko::memory;
using kinoko::script::diagnostic_address;
using kinoko::script::diagnostic_name;
using kinoko::native::RecordView;
using RuntimeView = RecordView<RuntimeRecord>;

int32_t float_bits(float value) { return load<int32_t>(&value); }
void* method(const void* object, unsigned index) {
    const auto table = load<const unsigned char*>(object);
    return table ? load<void*>(table + index * sizeof(void*)) : nullptr;
}
class DrawTarget final {
    bool selected_;
public:
    DrawTarget(KinokoActResource* target, kinoko::graphics::Device* device) : selected_(target != 0) {
        if (!selected_) return;
        kinoko_set_render_target(texture_runtime_state(target).handle);
        // 452636/452659 clear the selected target to opaque black before
        // checking stage/ACT visibility. The original ignores these HRESULTs.
        if (device) device->Clear(0, nullptr, kinoko::graphics::clear_target, 0xff000000u, 1.0f, 0);
    }
    ~DrawTarget() { if (selected_) kinoko_set_render_target(0); }
    DrawTarget(const DrawTarget&) = delete;
    DrawTarget& operator=(const DrawTarget&) = delete;
};
// Original 452670..452720 / 4529FB..452A84 save these states around the
// whole ACT pass, including layout virtual calls, not only BitBlt sprites.
void set_blend(kinoko::graphics::Device* device, int32_t blend) {
    using namespace kinoko::render;
    auto src=BlendFactor::one,dest=BlendFactor::zero;
    auto op=BlendOperation::add;
    switch(blend) {
    case 1: src=BlendFactor::source_alpha;dest=BlendFactor::inverse_source_alpha;break;
    case 2: src=BlendFactor::source_alpha;dest=BlendFactor::one;break;
    case 3: src=BlendFactor::source_alpha;dest=BlendFactor::one;op=BlendOperation::reverse_subtract;break;
    case 4: src=BlendFactor::zero;dest=BlendFactor::source_color;break;
    case 5: src=BlendFactor::destination_color;dest=BlendFactor::one;break;
    }
    GraphicsBlendSink sink(*device);
    sink.set_source(src);sink.set_destination(dest);sink.set_operation(op);
}
int32_t prepare_sprite(void* item, const BlitCommand& command) {
    if (command.texture <= 0 || static_cast<uint32_t>(command.texture) >= KINOKO_TEXTURE_CAPACITY) return kinoko::graphics::error_failure;
    const auto& texture = kinoko_texture_slots[command.texture];
    if (!texture.width || !texture.height) return kinoko::graphics::error_failure;
    KinokoSprite sprite{};
    sprite.vtable = const_cast<void*>(kinoko_act_host_symbols()->sprite_vtable);
    kinoko_sprite_set_rect(&sprite, nullptr, command.texture, command.source_x,
        command.source_y, command.width, command.height);
    const uint32_t color = (static_cast<uint32_t>(command.alpha * 255.0f) << 24) | 0xffffffu;
    for (auto& vertex : sprite.vertices) vertex.color = color;
    const RecordView<BlitSprite> target(item);
    target.set(&BlitSprite::command, command);
    target.set(&BlitSprite::sprite, sprite);
    return 0;
}
void trace_draw(KinokoActRuntime* self, const RuntimeView& resource, int32_t actor_index, int32_t trace_index) {
    const auto act = resource.get(&RuntimeRecord::active_document);
    const DocumentView document(act);
    if (actor_index <= 64) {
        kinoko_trace_i32("4525d0:actor-index", actor_index);
        kinoko_trace_i32("4525d0:flag-68", load<int32_t>(resource.bytes(&RuntimeRecord::hidden)));
        kinoko_trace_i32("4525d0:flag-8", load<int32_t>(resource.bytes(&RuntimeRecord::stage_active)));
        kinoko_trace_i32("4525d0:field-10", diagnostic_address(resource.get(&RuntimeRecord::active_holder)));
        kinoko_trace_i32("4525d0:act", diagnostic_address(act));
        if (act) {
            diagnostic_name("4525d0:actor-name", kinoko_string_data((const void*)(document.bytes(&DocumentRecord::name))));
            kinoko_trace_i32("4525d0:act-60", load<int32_t>(document.bytes(&DocumentRecord::visible)));
            kinoko_trace_i32("4525d0:act-begin", diagnostic_address(document.get(&DocumentRecord::layers).begin));
            kinoko_trace_i32("4525d0:act-end", diagnostic_address(document.get(&DocumentRecord::layers).end));
        }
    }
    if (trace_index <= 8) {
        kinoko_trace("4525d0:live-entry");
        kinoko_trace_i32("4525d0:live-resource", diagnostic_address(self));
        kinoko_trace_i32("4525d0:live-act", diagnostic_address(act));
        if (act) diagnostic_name("4525d0:live-act-name", kinoko_string_data((const void*)(document.bytes(&DocumentRecord::name))));
        diagnostic_name("4525d0:live-resource-name", kinoko_string_data((const void*)(resource.bytes(&RuntimeRecord::name))));
    }
}
}

extern "C" int32_t kinoko_act_prepare_draw(KinokoActRuntime* self) {
    if (!self) return kinoko::graphics::error_failure;
    const RuntimeView resource(self);
    if (resource.get(&RuntimeRecord::hidden)) return 0;
    kinoko::runtime::Lock lock(resource.get(&RuntimeRecord::lock));
    const auto act = resource.get(&RuntimeRecord::active_document);
    if (!resource.get(&RuntimeRecord::stage_active) || !act) return 0;
    const DocumentView document(act);
    if (!document.get(&DocumentRecord::visible)) return 0;
    int32_t result = 0;
    const auto layers = document.get(&DocumentRecord::layers);
    for (int32_t i = layer_distance(layers) - 1; i >= 0; --i) {
        const auto layout = kinoko_act_layer_layout(self, i);
        if (layout) {
            if (LayoutMethods(layout).update() < 0) result = kinoko::graphics::error_failure;
        }
    }
    const auto commands = kinoko_act_command_span((KinokoActRuntime*)(intptr_t)(self));
    const auto count = commands.begin ? static_cast<int32_t>((commands.end - commands.begin) / sizeof(BlitCommand)) : 0;
    kinoko_act_resize_sprites((KinokoActSpriteStorage*)(resource.bytes(&RuntimeRecord::draw_sprites)), count);
    const auto sprites = kinoko_act_sprite_span((KinokoActRuntime*)(intptr_t)(self));
    if ((sprites.begin ? static_cast<int32_t>((sprites.end - sprites.begin) / sizeof(BlitSprite)) : 0) != count) return kinoko::graphics::error_out_of_memory;
    for (int32_t i = 0; i < count; ++i)
        if (prepare_sprite(sprites.begin + i * sizeof(BlitSprite),
                load<BlitCommand>(commands.begin + i * sizeof(BlitCommand))) < 0) result = kinoko::graphics::error_failure;
    return result;
}

extern "C" int32_t kinoko_act_draw(KinokoActRuntime* self, float x, float y) {
    auto* device = kinoko_graphics.device;
    if (!self) return kinoko::graphics::error_failure;
    const RuntimeView resource(self);
    if (resource.get(&RuntimeRecord::hidden)) return 0;
    static std::atomic<int32_t> actor_trace_count, trace_count;
    const auto actor_index = ++actor_trace_count;
    const auto trace_index = ++trace_count;
    trace_draw(self, resource, actor_index, trace_index);
    kinoko::runtime::Lock lock(resource.get(&RuntimeRecord::lock));
    DrawTarget target(resource.get(&RuntimeRecord::render_target), device);
    if (!resource.get(&RuntimeRecord::stage_active)) return 0;
    const auto act = resource.get(&RuntimeRecord::active_document);
    if (!act) return kinoko::graphics::error_failure;
    const DocumentView document(act);
    if (!document.get(&DocumentRecord::visible)) return 0;
    const auto layers = document.get(&DocumentRecord::layers);
    if (!ordered_layers(layers)) return 0;
    const float draw_x = x + document.get(&DocumentRecord::offset_x);
    const float draw_y = y + document.get(&DocumentRecord::offset_y);
    kinoko::render::GraphicsDrawState states(device,kinoko::render::ScopeKind::act_pass);
    int32_t result = 0;
    for (int32_t i = layer_distance(layers) - 1; i >= 0; --i) {
        const auto layout = kinoko_act_layer_layout(self, i);
        if (!layout) continue;
        const auto status = LayoutMethods(layout).draw(draw_x, draw_y);
        if (trace_index <= 8) {
            kinoko_trace_i32("4525d0:live-x", float_bits(draw_x));
            kinoko_trace_i32("4525d0:live-y", float_bits(draw_y));
            kinoko_trace_i32("4525d0:live-layout", diagnostic_address(layout));
            kinoko_trace_i32("4525d0:live-texture", load<const void*>(layout)==kinoko_string_layout_methods()?0:RecordView<Layout2DRecord>(layout).get(&Layout2DRecord::texture));
            kinoko_trace_i32("4525d0:live-draw-result", status);
        }
        if (status < 0) result = status;
    }
    if (document.get(&DocumentRecord::visible)) {
        auto* blit_device = kinoko_graphics.device;
        if (blit_device) {
            for (auto item = kinoko_act_sprite_span((KinokoActRuntime*)(intptr_t)(self)).begin;
                 item != kinoko_act_sprite_span((KinokoActRuntime*)(intptr_t)(self)).end; item += sizeof(BlitSprite)) {
                const RecordView<BlitSprite> entry(item);
                const auto command = entry.get(&BlitSprite::command);
                const auto sprite = entry.bytes(&BlitSprite::sprite);
                set_blend(blit_device, command.blend);
                const auto draw = method(sprite, 7);
                if (draw && kinoko::method::invoke<int32_t>(sprite, draw,
                    draw_x + command.x, draw_y + command.y) < 0) result = kinoko::graphics::error_failure;
            }
            kinoko_texture_bind_stage(0, 0);
        }
    }
    return result;
}
