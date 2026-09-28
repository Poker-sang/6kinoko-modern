#include "kinoko/actor_animation.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/animation_storage.h"



namespace {
using namespace kinoko::actor;
using kinoko::native::RecordView;

int32_t frame_count(const AnimationRecord& animation) {
    const auto bytes=reinterpret_cast<uintptr_t>(animation.frames_end)-reinterpret_cast<uintptr_t>(animation.frames_begin);
    return static_cast<int32_t>(static_cast<intptr_t>(bytes)/static_cast<intptr_t>(sizeof(FrameRecord)));
}
int32_t increment(int32_t value) {
    return static_cast<int32_t>(static_cast<uint32_t>(value) + 1u);
}
AnimationRecord animation_at(KinokoAnimation *value) {
    return RecordView<AnimationRecord>(value).load();
}
KinokoAnimationFrame *select_frame(const ActorView& actor, const AnimationRecord& animation,
                     int32_t index) {
    // Preserve the original upper-only clamp, including negative indices,
    // but perform the byte displacement at native pointer width.
    const auto offset=static_cast<intptr_t>(index)*static_cast<intptr_t>(sizeof(FrameRecord));
    auto* frame=reinterpret_cast<KinokoAnimationFrame*>(reinterpret_cast<uintptr_t>(animation.frames_begin)+static_cast<uintptr_t>(offset));
    actor.set(&ActorRecord::current_frame, frame);
    actor.set(&ActorRecord::sprite_frame, frame);
    return frame;
}
void update_bounds(const ActorView& actor, const AnimationRecord& animation) {
    const auto x = actor.get(&ActorRecord::x);
    const auto y = actor.get(&ActorRecord::y);
    const auto scale = actor.get(&ActorRecord::scale);
    const auto scale_x = actor.get(&ActorRecord::scale_x);
    const auto scale_y = actor.get(&ActorRecord::scale_y);
    Bounds local{};
    if (animation.has_bounds) {
        local = {
            static_cast<float>(static_cast<double>(animation.left) - 0.5),
            static_cast<float>(animation.top),
            static_cast<float>(static_cast<double>(animation.right) + 0.5),
            static_cast<float>(static_cast<double>(animation.bottom) + 1.0)};
        actor.set(&ActorRecord::bounds_anchor_x,
            static_cast<float>(static_cast<double>(animation.left) + x));
        actor.set(&ActorRecord::bounds_anchor_y,
            static_cast<float>(static_cast<double>(animation.top) + y));
    } else {
        actor.set(&ActorRecord::bounds_anchor_x, 0.0f);
        actor.set(&ActorRecord::bounds_anchor_y, 0.0f);
        const auto initial = actor.view(&ActorRecord::initial);
        initial.set(&InitialData::width, int16_t{0});
        initial.set(&InitialData::height, int16_t{0});
    }
    actor.set(&ActorRecord::local_bounds, local);
    // Keep original double intermediates, operation order and half-pixel rules.
    Bounds world{};
    if (actor.get(&ActorRecord::direction) <= 0) {
        world.left = static_cast<float>(static_cast<double>(local.left) * scale * scale_x + x);
        world.right = static_cast<float>(static_cast<double>(local.right) * scale * scale_x + x);
    } else {
        world.left = static_cast<float>(x - static_cast<double>(local.right) * scale * scale_x);
        world.right = static_cast<float>(x - static_cast<double>(local.left) * scale * scale_x);
    }
    world.top = static_cast<float>(static_cast<double>(local.top) * scale * scale_y + y);
    world.bottom = static_cast<float>(static_cast<double>(local.bottom) * scale * scale_y + y);
    actor.set(&ActorRecord::world_bounds, world);
    const auto initial = actor.view(&ActorRecord::initial);
    initial.set(&InitialData::width, static_cast<int16_t>(static_cast<int32_t>(world.right - world.left)));
    initial.set(&InitialData::height, static_cast<int16_t>(static_cast<int32_t>(world.bottom - world.top)));
}
}

// 462280: SetTake updates state before lookup, including failed lookups.
extern "C" void* kinoko_actor_set_take(KinokoActor *value, int32_t take) {
    const ActorView actor(value);
    const ManagerView manager(actor.get(&ActorRecord::manager));
    actor.set(&ActorRecord::take, take);
    actor.set(&ActorRecord::frame_index, int32_t{0});
    actor.set(&ActorRecord::frame_time, int32_t{0});
    const auto lookup = manager.view(&ManagerPrefix::animation_lookup);
    auto *selected=kinoko_animation_find(reinterpret_cast<KinokoActorManager *>(manager.data()),take);
    if (!selected) return lookup.get(&AnimationIndex::owner);
    actor.set(&ActorRecord::animation, selected);
    const auto animation = animation_at(selected);
    actor.set(&ActorRecord::take_duration, animation.duration_total);
    update_bounds(actor, animation);
    return select_frame(actor, animation, 0);
}
extern "C" void* __fastcall kinoko_actor_set_take_method(KinokoActor *actor, void*, int32_t take) {
    return kinoko_actor_set_take(actor, take);
}

// A changed take defers ticking until the next update, as in 45E120.
extern "C" void kinoko_actor_advance_animation(KinokoActor *value, int32_t take_before_callback) {
    const ActorView actor(value);
    const auto current = actor.get(&ActorRecord::current_frame);
    if (!current || actor.get(&ActorRecord::take) != take_before_callback) return;
    const auto time = increment(actor.get(&ActorRecord::frame_time));
    actor.set(&ActorRecord::frame_time, time);
    const RecordView<FrameRecord> frame(current);
    const auto animation_address = actor.get(&ActorRecord::animation);
    if (time < frame.get(&FrameRecord::duration) || !animation_address) return;
    const auto animation = animation_at(animation_address);
    const auto count = frame_count(animation);
    if (count <= 0) return;
    const auto next = increment(actor.get(&ActorRecord::frame_index));
    actor.set(&ActorRecord::frame_index, next);
    if (next == count) {
        if (animation.loops) {
            actor.set(&ActorRecord::frame_index, int32_t{0});
        } else {
            actor.set(&ActorRecord::frame_index, next - 1);
            actor.set(&ActorRecord::frame_time, int32_t{0});
            return;
        }
    }
    select_frame(actor, animation, actor.get(&ActorRecord::frame_index));
    actor.set(&ActorRecord::frame_time, int32_t{0});
}

// 45FE80 has only an upper-bound clamp: do not invent a lower-bound policy.
extern "C" void kinoko_actor_sync_animation_state(KinokoActor *value, KinokoActor *source_value) {
    const ActorView actor(value), source(source_value);
    auto frame = source.get(&ActorRecord::frame_index);
    actor.set(&ActorRecord::frame_time, source.get(&ActorRecord::frame_time));
    actor.set(&ActorRecord::frame_index, frame);
    const auto animation = animation_at(actor.get(&ActorRecord::animation));
    const auto count = frame_count(animation);
    if (frame >= count) {
        frame = count - 1;
        if (frame < 0) frame = 0;
        actor.set(&ActorRecord::frame_index, frame);
    }
    select_frame(actor, animation, frame);
}
