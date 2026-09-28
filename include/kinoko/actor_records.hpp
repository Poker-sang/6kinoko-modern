#pragma once
#include "kinoko/integer_map.h"
#include "kinoko/owned_script_object.h"
#include "kinoko/integer_vector.h"
#include "kinoko/native_record_view.hpp"
#include "kinoko/native_control.hpp"
#include "kinoko/sprite.h"
#include "kinoko/quad_records.hpp"
#include "kinoko/camera_records.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

struct KinokoActor;
struct KinokoActorPool;
struct KinokoActorManager;
struct KinokoAnimation;
struct KinokoAnimationLookup;
struct KinokoAnimationFrame;
struct SQVM;

namespace kinoko::actor {
using ScriptStorage = std::array<unsigned char, sizeof(KinokoOwnedObjectWords)>; // external refs: ObjectView
using Bounds = kinoko::camera::Bounds;
struct InitialData {
    std::array<unsigned char, 12> unknown;
    std::int16_t width, height;
    std::int64_t chip_flags;
    std::array<unsigned char, 10> unknown24;
    std::uint16_t chip_bound_type;
    std::array<unsigned char, 12> unknown36;
};
// One schema shared by constructor, callbacks, animation and actor methods.
// Opaque bytes are deliberately not assigned speculative semantics.
struct ActorRecord {
    const void *vtable;
    std::array<unsigned char, 4> unknown4;
    std::int32_t owner_references;
    uint32_t pool_handle;
    void *priority_entry; // borrowed token; priority index owns it
    std::uint8_t registration_flag20, visible;
    std::uint8_t release_pending, unknown23;
    KinokoActor **owner;
    native::ControlRecord *owner_control; // strong native control; owns the Actor* slot
    KinokoActor **step;
    native::ControlRecord *step_control; // weak native control; never owns the Actor
    std::uint8_t active;
    std::array<unsigned char, 3> unknown41;
    alignas(void*) ScriptStorage script_object, initial_function, initial_argument; // saved Init inputs
    float spawn_x, spawn_y, spawn_z;
    SQVM *update_vm;
    ScriptStorage update_environment, update_function;
    SQVM *collision_vm; // callback state prefix, borrowed
    ScriptStorage collision_environment, collision_function;
    KinokoActorManager *manager;              // borrowed; manager owns the live Actor set
    KinokoAnimationFrame *sprite_frame; // borrowed from manager-owned animation frames
    float offset_x, offset_y, rotation;
    float scale, scale_x, scale_y;
    int32_t alpha, red, green, blue, blend;
    KinokoAnimation *animation; // borrowed, never freed by Actor
    KinokoAnimationFrame *current_frame;
    std::int32_t take, frame_index, frame_time, take_duration;
    int32_t id;
    std::int32_t priority;
    std::uint32_t update_group, flags;
    float x, y;
    float previous_x, previous_y;
    float velocity_x, velocity_y, parent_velocity_x, parent_velocity_y;
    float direction;
    float pitch, pitch_top;
    std::array<int32_t, 4> hits; // left, top, right, bottom
    uint8_t crushed;
    std::array<uint8_t, 3> padding301;
    float free_width, free_height;
    std::uint32_t collision_group, collision_mask, callback_group, callback_mask;
    const unsigned char *collision_chip;
    const void *collision_placement;
    std::int32_t collision_index;
    std::array<unsigned char, 12> inline_slots;
    float bounds_anchor_x, bounds_anchor_y;
    std::array<unsigned char, 12> unknown360;
    uint32_t unknown372;
    InitialData initial;
    Bounds local_bounds, world_bounds;
    Bounds previous_bounds;
    std::uint32_t collision_flags, unknown476;
    std::array<int32_t, 8> collision_scan_cache;
    // Remaining storage begins with the layer cache. No new bound check or
    // guessed supported layer count is imposed on the original GetChipID ABI.
    std::array<unsigned char, 32> chip_cache_storage;
};
using native::ControlRecord;
using native::ControlTable;
struct AnimationRecord {
    KinokoAnimation *next, *previous; // borrowed links; owning animation list is separate
    KinokoAnimationFrame *frames_begin, *frames_end, *frames_capacity;
    uint32_t unknown20;
    std::uint8_t loops, has_bounds;
    std::array<unsigned char, 2> unknown26;
    std::int32_t left, top, right, bottom, duration_total;
};
struct FrameAppearance {
    int32_t blend;
    uint32_t color;
    float scale_x, scale_y, roll_x, roll_y, roll_z;
};
using Position3 = kinoko::render::Position3;
struct FrameRecord {
    const void* vtable;
    std::int32_t texture; // borrowed handle; manager releases texture owners
    std::array<KinokoSpriteVertex, 4> vertices;
    float texture_width, texture_height;
    std::array<Position3, 4> base_positions, positions;
    float source_u_extent, source_v_extent;
    int16_t sprite_x, sprite_y, pivot_x, pivot_y;
    std::int16_t duration;
    std::uint16_t unknown242;
    FrameAppearance *owned_payload; // malloc-owned, released before frame storage
};
struct TreeIndex {
    void *policy, *head;
    std::int32_t count;
};
struct ListIndex { void* head; std::uint32_t count; };
using VectorIndex = KinokoIntegerVector;
// Manager iteration storage is native_buffer-owned; entries borrow live Actors.
struct ActorIterationBuffer { KinokoActor **begin, **end; void *storage_owner; };
struct RenderLayerRecord {
    const void *methods;
    KinokoActorManager *manager;
    int32_t index, begin, end;
};
using CameraBoundsRecord = kinoko::camera::Record;
// Only the verified prefix of the manager is described, not a new allocation
// size. Index nodes and animation lists have distinct ownership semantics.
struct AnimationIndex { uint32_t policy; KinokoAnimationLookup* owner; int32_t count; };
static_assert(offsetof(AnimationIndex,owner) % alignof(void*) == 0);
struct ManagerPrefix {
    const void *methods;
    KinokoActorPool *pool;
    void *owner_list;
    std::array<unsigned char, 8> unknown12;
    std::array<RenderLayerRecord *, 4> render_layers;
    AnimationIndex animation_lookup; // owns nodes; values borrow animation list items
    std::array<unsigned char, 4> unknown48;
    ListIndex animations;       // owns animation items, frames and payloads
    uint32_t unknown60;
    int32_t update_mask;
    VectorIndex textures;      // owns texture handles, retains vector capacity
    std::array<unsigned char, 4> unknown80;
    TreeIndex actors;          // live actor/priority tree
    std::array<unsigned char, 4> unknown96;
    ActorIterationBuffer iteration;
    std::array<unsigned char, 4> unknown112;
    std::int32_t iteration_count;
    std::uint8_t cleanup_pending;
    std::array<unsigned char, 3> unknown121;
    ActorIterationBuffer callback_candidates;
};
using ActorView = native::RecordView<ActorRecord>;
using ManagerView = native::RecordView<ManagerPrefix>;
inline constexpr std::array<ScriptStorage ActorRecord::*, 7> script_members{
    &ActorRecord::script_object, &ActorRecord::initial_function,
    &ActorRecord::initial_argument, &ActorRecord::update_environment,
    &ActorRecord::update_function, &ActorRecord::collision_environment, &ActorRecord::collision_function};

static_assert(sizeof(InitialData)==48 && sizeof(FrameAppearance)==28);
static_assert(offsetof(InitialData,chip_flags)==16 && offsetof(InitialData,chip_bound_type)==34);
static_assert(sizeof(ScriptStorage)==sizeof(KinokoOwnedObjectWords));
static_assert(offsetof(ActorRecord,update_environment)==offsetof(ActorRecord,update_vm)+sizeof(void*));
static_assert(offsetof(ActorRecord,update_function)==offsetof(ActorRecord,update_environment)+sizeof(ScriptStorage));
static_assert(offsetof(ActorRecord,collision_function)==offsetof(ActorRecord,collision_environment)+sizeof(ScriptStorage));
static_assert(sizeof(ActorIterationBuffer)==3*sizeof(void*));
// PAT frame and generic quad consumers share field geometry, never fixed bytes.
#define KINOKO_FRAME_PREFIX(member) static_assert(offsetof(FrameRecord,member)==offsetof(render::QuadRecord,member))
KINOKO_FRAME_PREFIX(texture); KINOKO_FRAME_PREFIX(vertices);
KINOKO_FRAME_PREFIX(base_positions); KINOKO_FRAME_PREFIX(positions);
KINOKO_FRAME_PREFIX(source_v_extent);
#undef KINOKO_FRAME_PREFIX
#if INTPTR_MAX == INT32_MAX
#include "kinoko/actor_x86_layout_checks.hpp"
#endif
}
