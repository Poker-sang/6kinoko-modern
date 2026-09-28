#include "kinoko/scene_queue.h"
#include "kinoko/application_runtime.hpp"
#include <list>
#include <stdexcept>

namespace kinoko::application {
namespace {
std::list<Scene*> retired_scenes;
using CriticalLock=kinoko::runtime::Lock;
}
void activate_pending_scene() {
    Scene* previous;
    {
        CriticalLock lock(&state.scene_lock);
        if (!state.pending_scene) return;
        previous = state.scene;
        state.scene = state.pending_scene;
        state.pending_scene = nullptr;
    }
    if (previous) {
        previous->methods->leave(previous, nullptr, state.requested_scene);
        {
            // Protect native list mutations, but retain original callback order.
            CriticalLock lock(&state.scene_lock);
            if (retired_scenes.size() == 0x3ffffffeu) throw std::length_error("list<T> too long");
            retired_scenes.push_back(previous);
        }
        if (state.retire_event) kinoko::runtime::signal(state.retire_event.get());
    }
    if (state.scene) state.scene->methods->enter(state.scene, nullptr, state.current_scene);
    state.current_scene = state.requested_scene;
}
}
extern "C" void kinoko_initialize_scene_queue() {
    kinoko::application::retired_scenes.clear();
}
extern "C" void kinoko_destroy_retired_scenes() {
    using namespace kinoko::application;
    for (;;) {
        Scene* object;
        {
            kinoko::runtime::Lock lock(&state.scene_lock);
            if (retired_scenes.empty()) return;
            object = retired_scenes.front();
        }
        // Single consumer; deleting destructor precedes removing its queue node.
        if (object) object->methods->destroy(object, nullptr, 1);
        {
            kinoko::runtime::Lock lock(&state.scene_lock);
            retired_scenes.pop_front();
        }
    }
}
