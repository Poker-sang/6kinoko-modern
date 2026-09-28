#include "kinoko/runtime_util.hpp"
// Diagnostic observations only; no gameplay decisions or VM writes.
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_manager.h"
#include "kinoko/actor_render.h"
#include "kinoko/map_collision.h"
#include "kinoko/game_host.h"
#include "kinoko/map_manager.h"
#include "kinoko/application.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/memory_access.hpp"
#include "kinoko/script_diagnostics.hpp"
#include <cstdio>
#include <cstring>
extern "C" {
void kinoko_trace(const char *);
void kinoko_trace_i32(const char *,int32_t);
void kinoko_trace_squirrel_name(const char *,int32_t);
}
inline void kinoko_trace_i32(const char* label, const void* value) { kinoko_trace_i32(label, kinoko::script::diagnostic_address(value)); }
using namespace kinoko::actor;
namespace {
template<class T> const unsigned char *actor_bytes(KinokoActor* actor,T ActorRecord::*member,size_t inner=0) {
    return ActorView(actor).bytes(member)+inner;
}
template<class T,class M> T actor_bits(KinokoActor* actor,M ActorRecord::*member,size_t inner=0) {
    return kinoko::memory::load<T>(actor_bytes(actor,member,inner));
}
template<class T,class M> T camera_bits(KinokoCamera *camera,M kinoko::camera::Record::*member,size_t inner=0) {
    return kinoko::memory::load<T>(kinoko::camera::View(camera).bytes(member)+inner);
}
}
extern "C" void kinoko_trace_star_state(const char *phase, KinokoActor* actor) {
    static struct { uint32_t handle; int hits, samples; } observed[32];
    uint32_t handle;
    FrameRecord* sprite; SQFunctionProto* proto=nullptr;
    int release, hits, index;
    char message[768];
    if (!actor || actor_bits<int32_t>(actor, &ActorRecord::take)!=1060) return;
    release=strcmp(phase,"release")==0;
    if (actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, type))==0x08000100)
        proto=_funcproto(actor_bits<SQClosure*>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value))->_function);
    if (!release && (!proto || proto->_name._type!=0x08000010 ||
        strcmp(_stringval(proto->_name),"UpdateWalk")!=0)) return;
    handle=actor_bits<uint32_t>(actor, &ActorRecord::pool_handle);
    index=(int)(handle%32);
    hits=actor_bits<int32_t>(actor, &ActorRecord::hits, 3 * sizeof(int32_t));
    if(observed[index].handle!=handle) {
        observed[index].handle=handle;
        observed[index].samples=0;
        observed[index].hits=-1;
    }
    if(!release && ((observed[index].hits==hits && kinoko_application_frame_count()%10!=0) || observed[index].samples>=256)) return;
    observed[index].hits=hits;
    ++observed[index].samples;
    sprite=actor_bits<FrameRecord*>(actor, &ActorRecord::current_frame);
    std::snprintf(message,sizeof(message),
        "actor:star frame=%d phase=%s handle=%08X xy=(%.6g,%.6g) v=(%.6g,%.6g) "
        "hits=(%d,%d,%d,%d) bounds=(%.6g,%.6g,%.6g,%.6g) "
        "active=%d visible=%d release=%d priority=%d alpha=%d texture=%d spriteY=(%.6g,%.6g) "
        "mapHeight=%d camera=(%.6g,%.6g,%.6g,%.6g)",
        kinoko_application_frame_count(),phase,handle,actor_bits<float>(actor, &ActorRecord::x),actor_bits<float>(actor, &ActorRecord::y),
        actor_bits<float>(actor, &ActorRecord::velocity_x),actor_bits<float>(actor, &ActorRecord::velocity_y),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 0 * sizeof(int32_t)),actor_bits<int32_t>(actor, &ActorRecord::hits, 1 * sizeof(int32_t)),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 2 * sizeof(int32_t)),hits,
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)),actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)),
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)),actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)),
        actor_bits<uint8_t>(actor, &ActorRecord::active),actor_bits<uint8_t>(actor, &ActorRecord::visible),
        actor_bits<uint8_t>(actor, &ActorRecord::release_pending),actor_bits<int32_t>(actor, &ActorRecord::priority),
        actor_bits<int32_t>(actor, &ActorRecord::alpha),sprite ? kinoko::native::RecordView<FrameRecord>(sprite).get(&FrameRecord::texture) : 0,
        sprite ? kinoko::native::RecordView<FrameRecord>(sprite).get(&FrameRecord::positions)[0].y : 0,sprite ? kinoko::native::RecordView<FrameRecord>(sprite).get(&FrameRecord::positions)[3].y : 0,
        kinoko_map_manager_height(kinoko_game_objects()->map),
        camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, left)),camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, top)),
        camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, right)),camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)));
    kinoko_trace(message);
    if (release) {
        auto* vm = kinoko_actor_default_vm();
        if (!vm) return;
        const auto frames=vm->_callsstacksize;
        for (SQInteger i=frames-1;i>=0 && i>=frames-5;--i) {
            const auto& frame=vm->_callsstack[i];
            if (sq_type(frame._closure)==OT_CLOSURE) {
                auto *caller=_funcproto(_closure(frame._closure)->_function);
                kinoko::script::diagnostic_name("actor:star-release-source",sq_type(caller->_sourcename)==OT_STRING?_stringval(caller->_sourcename):"<unknown>");
                kinoko::script::diagnostic_name("actor:star-release-function",sq_type(caller->_name)==OT_STRING?_stringval(caller->_name):"<anonymous>");
                kinoko_trace_i32("actor:star-release-instruction",static_cast<int32_t>(frame._ip-caller->_instructions)-1);
            }
        }
    }
}

static void kinoko_trace_invalid_actor(const char *phase, KinokoActor* actor) {
    static std::atomic<int32_t> count;
    uint32_t bits[12];
    int invalid=0;
    if(!actor) return;
    const unsigned char *fields[]={actor_bytes(actor, &ActorRecord::x), actor_bytes(actor, &ActorRecord::y), actor_bytes(actor, &ActorRecord::velocity_x), actor_bytes(actor, &ActorRecord::velocity_y), actor_bytes(actor, &ActorRecord::parent_velocity_x), actor_bytes(actor, &ActorRecord::parent_velocity_y), actor_bytes(actor, &ActorRecord::free_width), actor_bytes(actor, &ActorRecord::free_height), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)), actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom))};

    for(int i=0;i<12;++i) {
        memcpy(bits+i,fields[i],4);
        invalid |= (bits[i]&0x7f800000u)==0x7f800000u;
    }
    if(invalid && ++count<=32) {
        char message[512];
        std::snprintf(message,sizeof(message),
            "actor:invalid-state frame=%d phase=%s actor=%08X take=%d "
            "xy=%08X,%08X v=%08X,%08X parentDelta=%08X,%08X free=%08X,%08X "
            "bounds=%08X,%08X,%08X,%08X step=%08X,%08X",
            kinoko_application_frame_count(),phase,static_cast<uint32_t>(reinterpret_cast<uintptr_t>(actor)),actor_bits<int32_t>(actor, &ActorRecord::take),
            bits[0],bits[1],bits[2],bits[3],bits[4],bits[5],bits[6],bits[7],
            bits[8],bits[9],bits[10],bits[11],
            actor_bits<uint32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, type)),actor_bits<uint32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value)));
        kinoko_trace(message);
    }
}

extern "C" int32_t kinoko_actor_trace_step_begin(KinokoActor *receiver, int32_t callback_type) {
    KinokoActor* const actor = receiver;
    static std::atomic<int32_t> step_trace_count;
    int32_t step_trace_index;
    step_trace_index = ++step_trace_count;
    if (step_trace_index <= 64 &&
        actor_bits<int32_t>(actor, &ActorRecord::id) >= 0x200 &&
        actor_bits<int32_t>(actor, &ActorRecord::id) <= 0x207) {
        int32_t y_bits;

        memcpy(&y_bits, actor_bytes(actor, &ActorRecord::y),
               sizeof(y_bits));
        kinoko_trace_i32("actor:step-id",
                         actor_bits<int32_t>(actor, &ActorRecord::id));
        kinoko_trace_i32("actor:step-type", callback_type);
        kinoko_trace_i32("actor:step-data",
                         actor_bits<SQClosure*>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value)));
        kinoko_trace_i32("actor:step-state-vm",
                         actor_bits<int32_t>(actor, &ActorRecord::update_vm));
        kinoko_trace_i32("actor:step-state-type",
                         actor_bits<int32_t>(actor, &ActorRecord::update_environment, offsetof(KinokoOwnedObjectWords, type)));
        kinoko_trace_i32("actor:step-state-data",
                         actor_bits<int32_t>(actor, &ActorRecord::update_environment, offsetof(KinokoOwnedObjectWords, value)));
        kinoko_trace_i32("actor:step-callback-data",
                         actor_bits<int32_t>(actor, &ActorRecord::step));
        kinoko_trace_i32("actor:step-callback-delegate",
                         actor_bits<int32_t>(actor, &ActorRecord::step_control));
        kinoko_trace_i32("actor:step-y-before", y_bits);
    }
    if (callback_type == 0x08000100) kinoko_trace_invalid_actor("before-script",actor);
    return step_trace_index;
}

extern "C" void kinoko_actor_trace_step_end(KinokoActor *receiver, int32_t step_result, int32_t step_trace_index) {
    KinokoActor* const actor = receiver;
        kinoko_trace_invalid_actor("after-script",actor);
        /* Original 45E180 failure retirement is handled by the C++ adapter. */
        if (step_result < 0) {
            static std::atomic<int32_t> failure_count;
            if (++failure_count <= 64) {
                char message[384];
                std::snprintf(message, sizeof(message),
                    "actor:update-failed frame=%d actor=%08X id=%X take=%d "
                    "xy=(%.3f,%.3f) v=(%.3f,%.3f) camera=(%.3f,%.3f,%.3f,%.3f)",
                    kinoko_application_frame_count(), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(actor)), actor_bits<uint32_t>(actor, &ActorRecord::id),
                    actor_bits<int32_t>(actor, &ActorRecord::take),
                    actor_bits<float>(actor, &ActorRecord::x), actor_bits<float>(actor, &ActorRecord::y),
                    actor_bits<float>(actor, &ActorRecord::velocity_x), actor_bits<float>(actor, &ActorRecord::velocity_y),
                    camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, left)), camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, top)),
                    camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, right)), camera_bits<float>(kinoko_game_objects()->camera, &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)));
                kinoko_trace(message);
            }
        }
        if (step_trace_index <= 64)
            kinoko_trace_i32("actor:step-result", step_result);
        if (step_trace_index <= 64 &&
            actor_bits<int32_t>(actor, &ActorRecord::id) >= 0x200 &&
            actor_bits<int32_t>(actor, &ActorRecord::id) <= 0x207) {
            int32_t y_bits;

            memcpy(&y_bits, actor_bytes(actor, &ActorRecord::y),
                   sizeof(y_bits));
            kinoko_trace_i32("actor:step-y-after", y_bits);
        }

}

extern "C" void kinoko_actor_trace_motion(KinokoActor *receiver, int32_t phase) {
    KinokoActor* actor = receiver;
    static std::atomic<int32_t> trace_count;
    int32_t trace_index;
    if (phase == 0) {
        kinoko_trace_invalid_actor("before-motion", actor);
        kinoko_trace_star_state("before-motion", actor);
    } else if (phase == 1) {
    trace_index = ++trace_count;
    if (trace_index <= 16) {
        int32_t before_x;
        int32_t after_x;
        memcpy(&before_x, actor_bytes(actor, &ActorRecord::previous_x),
               sizeof(before_x));
        memcpy(&after_x, actor_bytes(actor, &ActorRecord::x),
               sizeof(after_x));
        kinoko_trace_i32("actor:motion-id",
                         actor_bits<int32_t>(actor, &ActorRecord::id));
        kinoko_trace_i32("actor:motion-before-x", before_x);
        kinoko_trace_i32("actor:motion-after-x", after_x);
    }
    } else {
        kinoko_trace_invalid_actor("after-motion", actor);
        kinoko_trace_star_state("after-motion", actor);
    }
}

static void kinoko_trace_actor_window_state(int32_t phase, KinokoActor* actor,
                                             int32_t update_mask)
{
    int32_t id;
    FrameRecord* frame;
    void* node;
    int32_t value;

    if (actor == 0 || kinoko_application_frame_count() < 540 || kinoko_application_frame_count() > 820 || (kinoko_application_frame_count() % 10) != 0)
        return;
    id = actor_bits<int32_t>(actor, &ActorRecord::id);
    if (id < 0x200 || id > 0x207)
        return;

    frame = actor_bits<FrameRecord*>(actor, &ActorRecord::current_frame);
    node = actor_bits<void*>(actor, &ActorRecord::animation);
    kinoko_trace_i32("actor:diag-frame-counter", kinoko_application_frame_count());
    kinoko_trace_i32("actor:diag-phase", phase);
    kinoko_trace_i32("actor:diag-id", id);
    kinoko_trace_i32("actor:diag-address", actor);
    kinoko_trace_i32("actor:diag-animation-key",
                     actor_bits<int32_t>(actor, &ActorRecord::take));
    kinoko_trace_i32("actor:diag-frame-pointer", frame);
    kinoko_trace_i32("actor:diag-node-pointer", node);
    kinoko_trace_i32("actor:diag-frame-index",
                     actor_bits<int32_t>(actor, &ActorRecord::frame_index));
    kinoko_trace_i32("actor:diag-timer",
                     actor_bits<int32_t>(actor, &ActorRecord::frame_time));
    value = frame != 0
        ? (int32_t)kinoko::native::RecordView<FrameRecord>(frame).get(&FrameRecord::duration) : 0;
    kinoko_trace_i32("actor:diag-duration", value);
    kinoko_trace_i32("actor:diag-active",
                     actor_bits<unsigned char>(actor, &ActorRecord::active));
    kinoko_trace_i32("actor:diag-visible",
                     actor_bits<unsigned char>(actor, &ActorRecord::registration_flag20));
    kinoko_trace_i32("actor:diag-removable",
                     actor_bits<unsigned char>(actor, &ActorRecord::release_pending));
    kinoko_trace_i32("actor:diag-update-flags",
                     actor_bits<int32_t>(actor, &ActorRecord::update_group));
    kinoko_trace_i32("actor:diag-update-mask", update_mask);
    kinoko_trace_i32("actor:diag-mask-hit",
                     (actor_bits<int32_t>(actor, &ActorRecord::update_group) & update_mask) != 0);
    memcpy(&value, actor_bytes(actor, &ActorRecord::x), sizeof(value));
    kinoko_trace_i32("actor:diag-x", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::y), sizeof(value));
    kinoko_trace_i32("actor:diag-y", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)), sizeof(value));
    kinoko_trace_i32("actor:diag-left", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)), sizeof(value));
    kinoko_trace_i32("actor:diag-top", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)), sizeof(value));
    kinoko_trace_i32("actor:diag-right", value);
    memcpy(&value, actor_bytes(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)), sizeof(value));
    kinoko_trace_i32("actor:diag-bottom", value);
}

static void kinoko_trace_player_state(const char *phase, KinokoActor* actor,
                                       KinokoCamera* camera) {
    static std::atomic<int32_t> count;
    static KinokoActor* last_actor; static int32_t last_take;
    SQClosure* closure; SQFunctionProto* proto; int32_t take, transition;
    uint32_t xy_bits[2];
    const char *source, *name;
    char message[896];
    if (actor == 0 || actor_bits<int32_t>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, type)) != 0x08000100)
        return;
    closure = actor_bits<SQClosure*>(actor, &ActorRecord::update_function, offsetof(KinokoOwnedObjectWords, value));
    proto = _funcproto(closure->_function);
    if (proto == 0 || proto->_sourcename._type != 0x08000010 ||
        proto->_name._type != 0x08000010)
        return;
    source = _stringval(proto->_sourcename);
    name = _stringval(proto->_name);
    if (kinoko::compare_asset_names(source, "data/script/player.nut") != 0 || strcmp(name, "Update") != 0)
        return;
    take = actor_bits<int32_t>(actor, &ActorRecord::take);
    transition = last_actor != actor || last_take != take;
    last_actor = actor;
    last_take = take;
    if (!transition && actor_bits<float>(actor, &ActorRecord::velocity_x) == 0 && actor_bits<float>(actor, &ActorRecord::velocity_y) == 0 &&
        actor_bits<float>(actor, &ActorRecord::free_width) >= 12 &&
        !(actor_bits<int32_t>(actor, &ActorRecord::hits, 1 * sizeof(int32_t)) && actor_bits<int32_t>(actor, &ActorRecord::hits, 3 * sizeof(int32_t))) &&
        kinoko_application_frame_count() % 60 != 0)
        return;
    if (++count > 6000 && !transition)
        return;
    memcpy(xy_bits, actor_bytes(actor, &ActorRecord::x), sizeof(xy_bits));
    /* Observe the inputs to the unchanged script death checks without touching the VM stack. */
    std::snprintf(message, sizeof(message),
        "actor:player-state frame=%d phase=%s actor=%08X take=%d xy=(%.3f,%.3f) "
        "v=(%.3f,%.3f) free=(%.3f,%.3f) hits=(%d,%d,%d,%d) flags=%08X "
        "bounds=(%.3f,%.3f,%.3f,%.3f) camera=(%.3f,%.3f,%.3f,%.3f) xyBits=%08X,%08X",
        kinoko_application_frame_count(), phase, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(actor)), actor_bits<int32_t>(actor, &ActorRecord::take),
        actor_bits<float>(actor, &ActorRecord::x), actor_bits<float>(actor, &ActorRecord::y),
        actor_bits<float>(actor, &ActorRecord::velocity_x), actor_bits<float>(actor, &ActorRecord::velocity_y),
        actor_bits<float>(actor, &ActorRecord::free_width), actor_bits<float>(actor, &ActorRecord::free_height),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 0 * sizeof(int32_t)), actor_bits<int32_t>(actor, &ActorRecord::hits, 1 * sizeof(int32_t)),
        actor_bits<int32_t>(actor, &ActorRecord::hits, 2 * sizeof(int32_t)), actor_bits<int32_t>(actor, &ActorRecord::hits, 3 * sizeof(int32_t)),
        actor_bits<uint32_t>(actor, &ActorRecord::collision_flags),
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)), actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)),
        actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)), actor_bits<float>(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)),
        camera ? camera_bits<float>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, left)) : 0,
        camera ? camera_bits<float>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, top)) : 0,
        camera ? camera_bits<float>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, right)) : 0,
        camera ? camera_bits<float>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)) : 0, xy_bits[0], xy_bits[1]);
    kinoko_trace(message);
}

extern "C" int32_t kinoko_actor_render_trace_begin(KinokoActor *receiver, KinokoCamera *camera_pointer) {
    KinokoActor* actor = receiver;
    KinokoCamera* camera = camera_pointer;
    FrameRecord* frame;
    static std::atomic<int32_t> trace_count;
    int32_t trace_index = ++trace_count;
    if (!actor) return trace_index;
    frame = actor_bits<FrameRecord*>(actor, &ActorRecord::current_frame);
    if (trace_index <= 16) {
        kinoko_trace_i32("actor:render-actor", actor);
        kinoko_trace_i32("actor:render-id",
                         actor_bits<int32_t>(actor, &ActorRecord::id));
        kinoko_trace_i32("actor:render-frame", frame);
        kinoko_trace_i32("actor:render-node",
                         actor_bits<int32_t>(actor, &ActorRecord::animation));
        kinoko_trace_i32("actor:render-handle",
                         frame != 0 ? kinoko::native::RecordView<FrameRecord>(frame).get(&FrameRecord::texture) : 0);
        kinoko_trace_i32("actor:render-active",
                         actor_bits<int32_t>(actor, &ActorRecord::active));
        kinoko_trace_i32("actor:render-visible",
                         actor_bits<int32_t>(actor, &ActorRecord::visible));
        kinoko_trace_i32("actor:render-left",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, left)));
        kinoko_trace_i32("actor:render-top",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, top)));
        kinoko_trace_i32("actor:render-right",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, right)));
        kinoko_trace_i32("actor:render-bottom",
                         actor_bits<int32_t>(actor, &ActorRecord::world_bounds, offsetof(Bounds, bottom)));
        kinoko_trace_i32("actor:render-camera-left",
                         camera != 0 ? camera_bits<int32_t>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, left)) : 0);
        kinoko_trace_i32("actor:render-camera-top",
                         camera != 0 ? camera_bits<int32_t>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, top)) : 0);
        kinoko_trace_i32("actor:render-camera-right",
                         camera != 0 ? camera_bits<int32_t>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, right)) : 0);
        kinoko_trace_i32("actor:render-camera-bottom",
                         camera != 0 ? camera_bits<int32_t>(camera, &kinoko::camera::Record::bounds, offsetof(Bounds, bottom)) : 0);
    }
    kinoko_trace_star_state("render-entry",actor);
    return trace_index;
}

extern "C" void kinoko_actor_render_trace_draw(KinokoActor *actor) { kinoko_trace_star_state("draw",actor); }

extern "C" void kinoko_actor_render_trace_end(int32_t index, int32_t result) {
    if (index <= 16) kinoko_trace_i32("actor:render-submit",result);
}

extern "C" void kinoko_actor_trace_collision(KinokoActor *actor, int32_t after) {
    kinoko_trace_invalid_actor(after ? "after-collision" : "before-collision", actor);
}

extern "C" void kinoko_actor_manager_trace_actor(int32_t phase, KinokoActor *actor, KinokoCamera *camera, int32_t mask) {
    kinoko_trace_actor_window_state(phase, actor, mask);
    kinoko_trace_player_state(phase == 1 ? "before-script" : phase == 2 ? "after-script" : "after-motion",
        actor, camera);
}

extern "C" int32_t kinoko_actor_render_layer_update(void *storage,KinokoCamera *camera) {
    static std::atomic<int32_t> trace_count;
    const auto trace=++trace_count;
    if (!storage) return 0;
    const kinoko::native::RecordView<RenderLayerRecord> layer(storage);
    if (trace<=16) {
        using kinoko::script::diagnostic_address;
        using Camera=kinoko::camera::Record;
        auto *global_camera=kinoko_game_objects()->camera;
        kinoko_trace_i32("actor:layer-render-this",diagnostic_address(storage));
        kinoko_trace_i32("actor:layer-render-arg",diagnostic_address(camera));
        kinoko_trace_i32("actor:layer-render-camera",diagnostic_address(global_camera));
        kinoko_trace_i32("actor:camera-x",camera_bits<int32_t>(global_camera,&Camera::x));
        kinoko_trace_i32("actor:camera-y",camera_bits<int32_t>(global_camera,&Camera::y));
        kinoko_trace_i32("actor:camera-cx",camera_bits<int32_t>(global_camera,&Camera::center_x));
        kinoko_trace_i32("actor:camera-cy",camera_bits<int32_t>(global_camera,&Camera::center_y));
        kinoko_trace_i32("actor:camera-width",camera_bits<int32_t>(global_camera,&Camera::width));
        kinoko_trace_i32("actor:camera-height",camera_bits<int32_t>(global_camera,&Camera::height));
    }
    return kinoko_actor_manager_render_layer(layer.get(&RenderLayerRecord::manager),camera,layer.get(&RenderLayerRecord::index));
}
