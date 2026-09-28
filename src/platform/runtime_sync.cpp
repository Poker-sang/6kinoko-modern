#include "kinoko/runtime_sync.hpp"
#include <SDL3/SDL_thread.h>
#include <chrono>
#include <condition_variable>
namespace kinoko::runtime {
struct Event { bool manual=false, signaled=false, closed=false; };
namespace {
struct Domain { std::mutex mutex; std::condition_variable changed; };
// Process-lifetime wait domain: global owners/atexit workers must be able to
// signal and join during teardown, independent of static destruction order.
Domain& domain() { static auto* instance=new Domain; return *instance; }
}
EventHandle make_event(bool manual,bool signaled) noexcept {
    try { (void)domain(); return std::make_shared<Event>(Event{manual,signaled,false}); }
    catch (...) { return {}; }
}
void signal(const EventHandle& event) {
    if (!event) return;
    auto& hub=domain();
    { std::lock_guard<std::mutex> lock(hub.mutex); if(!event->closed) event->signaled=true; }
    hub.changed.notify_all();
}
void reset_event(const EventHandle& event) {
    if(!event) return;
    std::lock_guard<std::mutex> lock(domain().mutex); event->signaled=false;
}
void close_event(const EventHandle& event) {
    if(!event) return;
    auto& hub=domain();
    { std::lock_guard<std::mutex> lock(hub.mutex); event->closed=true; }
    hub.changed.notify_all();
}
int wait_any(std::initializer_list<EventHandle> events,uint32_t timeout) {
    if(events.size()==0) return wait_failed;
    for(const auto& event:events) if(!event) return wait_failed;
    auto& hub=domain(); std::unique_lock<std::mutex> lock(hub.mutex);
    int selected=wait_timeout;
    const auto ready=[&] {
        int index=0;
        for(const auto& event:events) {
            if(event->closed) { selected=wait_failed; return true; }
            if(event->signaled) { selected=index; return true; }
            ++index;
        }
        return false;
    };
    if(timeout==infinite) hub.changed.wait(lock,ready);
    else if(!hub.changed.wait_for(lock,std::chrono::milliseconds(timeout),ready)) return wait_timeout;
    if(selected>=0) { const auto& event=*(events.begin()+selected); if(!event->manual) event->signaled=false; }
    return selected;
}
void set_current_priority(Priority priority) noexcept {
    if(priority==Priority::normal) return;
    SDL_SetCurrentThreadPriority(priority==Priority::low?SDL_THREAD_PRIORITY_LOW:SDL_THREAD_PRIORITY_TIME_CRITICAL);
}
}
