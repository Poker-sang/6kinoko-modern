#include "kinoko/runtime_sync.hpp"
#include "kinoko/runtime_clock.h"
#include "kinoko/timer_events.h"
#include <atomic>
#include <cstdio>
using namespace kinoko::runtime;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"failed: %s\n",#x); return 1; } } while(0)
int main() {
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
