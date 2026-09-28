#include "kinoko/timer_events.h"
#include "kinoko/runtime_sync.hpp"
#include "kinoko/runtime_clock.h"
#include <list>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <new>
struct KinokoFrameEvent { kinoko::runtime::EventHandle event; };
namespace {
using namespace kinoko::runtime;
struct FrameTimer {
    std::recursive_mutex lock;
    Thread thread;
    std::atomic<bool> running{false};
    bool initialized=false, period_requested=false;
    std::list<std::unique_ptr<KinokoFrameEvent>> events;
};
FrameTimer timer;
void timer_worker(void*) {
    while(timer.running.load()) {
        kinoko_clock_delay(16);
        Lock lock(&timer.lock);
        for(const auto& event:timer.events) signal(event->event);
    }
}
void shutdown() {
    if(!timer.initialized) return;
    timer.running.store(false);
    timer.thread.reset();
    { Lock lock(&timer.lock); for(const auto& event:timer.events) close_event(event->event); timer.events.clear(); }
    if(timer.period_requested) kinoko_clock_release_resolution();
    timer.initialized=timer.period_requested=false;
}
}
extern "C" void kinoko_frame_timer_initialize() noexcept(false) {
    if(timer.initialized) return;
    if(std::atexit(shutdown)!=0) throw std::bad_alloc();
    timer.initialized=true;
    timer.period_requested=kinoko_clock_request_resolution()!=0;
    timer.running.store(true);
    if(!timer.thread.start(timer_worker,nullptr,Priority::time_critical)) timer.running.store(false);
}
extern "C" KinokoFrameEvent* kinoko_frame_timer_register() {
    if(!timer.initialized || !timer.thread) return nullptr;
    Lock lock(&timer.lock);
    auto event=make_event();
    if(!event) return nullptr;
    auto token=std::make_unique<KinokoFrameEvent>(); token->event=std::move(event);
    auto* result=token.get(); timer.events.push_back(std::move(token)); return result;
}
extern "C" int32_t kinoko_frame_timer_unregister(KinokoFrameEvent* token) {
    if(!timer.initialized) return 0;
    Lock lock(&timer.lock);
    const auto found=std::find_if(timer.events.begin(),timer.events.end(),[=](const auto& event){return event.get()==token;});
    if(found==timer.events.end()) return 0;
    close_event((*found)->event); timer.events.erase(found); return 1;
}
extern "C" void kinoko_frame_timer_wait(KinokoFrameEvent* token) {
    // Preserve the original skip-on-busy registry rule. A copied shared event
    // keeps a concurrent unregister safe and wakes this waiter on closure.
    if(!timer.initialized || !timer.lock.try_lock()) return;
    EventHandle event;
    for(const auto& entry:timer.events) if(entry.get()==token) { event=entry->event; break; }
    timer.lock.unlock();
    if(event) wait(event);
}
