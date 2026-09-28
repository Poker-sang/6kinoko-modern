#include "kinoko/runtime_paths.h"
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include "kinoko/critical_section.h"
#include "kinoko/runtime_sync.hpp"
#include "kinoko/runtime_clock.h"
#include "kinoko/timer_events.h"
#include <atomic>
#include <cstdio>
using namespace kinoko::runtime;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"failed: %s\n",#x); return 1; } } while(0)
int main() {
    const auto previous_directory=std::filesystem::current_path();
    const bool directory_ready=kinoko_use_executable_directory()!=0;
    const bool directory_matches=directory_ready && std::filesystem::equivalent(
        std::filesystem::current_path(),std::filesystem::u8path(SDL_GetBasePath()));
    std::filesystem::current_path(previous_directory);
    CHECK(directory_matches);

    struct GuardedLock { uint32_t before; KinokoCriticalSection lock; uint32_t after; } storage{0xaabbccdd,{},0x12345678};
    auto* owned=kinoko_critical_section_construct(&storage.lock);
    owned->native->lock();
    CHECK(owned->native->try_lock());
    auto other_thread_can_acquire=[&] {
        bool acquired=false;
        std::thread worker([&] {
            acquired=owned->native->try_lock();
            if(acquired) owned->native->unlock();
        });
        worker.join();
        return acquired;
    };
    CHECK(!other_thread_can_acquire());
    owned->native->unlock();
    CHECK(!other_thread_can_acquire());
    owned->native->unlock();
    CHECK(other_thread_can_acquire());
    CHECK(owned->methods->destroy(owned,nullptr,0)==owned);
    CHECK(!owned->native);
    CHECK(storage.before==0xaabbccdd && storage.after==0x12345678);
    kinoko_critical_section_construct(owned);
    kinoko_critical_section_destruct(owned);

    auto automatic=make_event();
    CHECK(automatic);
    signal(automatic); signal(automatic);
    CHECK(wait(automatic,0)==0);
    CHECK(wait(automatic,0)==wait_timeout);
    auto manual=make_event(true,true);
    CHECK(wait(manual,0)==0 && wait(manual,0)==0);
    reset_event(manual); CHECK(wait(manual,0)==wait_timeout);
    signal(automatic); signal(manual);
    CHECK(wait_any({automatic,manual},0)==0);
    CHECK(wait_any({automatic,manual},0)==1);
    for (bool close : {false,true}) {
        auto event=make_event(), ready=make_event();
        std::atomic<int> result{99};
        Thread worker;
        CHECK(worker.start([&](void*) { signal(ready); result=wait(event,2000); }));
        const int ready_result=wait(ready,2000);
        if(close) close_event(event); else signal(event);
        worker.reset();
        CHECK(ready_result==0);
        CHECK(result.load()==(close?wait_failed:0));
        CHECK(!worker);
    }
    CHECK(wait({},0)==wait_failed);
    auto closed=make_event(); close_event(closed);
    CHECK(wait(closed,0)==wait_failed);
    const auto before=kinoko_clock_milliseconds();
    kinoko_clock_delay(1);
    CHECK(static_cast<uint32_t>(kinoko_clock_milliseconds()-before)<10000);
    kinoko_frame_timer_initialize();
    auto* frame=kinoko_frame_timer_register();
    CHECK(frame);
    CHECK(kinoko_frame_timer_unregister(frame)==1);
    CHECK(kinoko_frame_timer_unregister(nullptr)==0);
    return 0;
}
