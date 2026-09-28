#pragma once
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <thread>
namespace kinoko::runtime {
struct Event;
using EventHandle=std::shared_ptr<Event>;
constexpr uint32_t infinite=UINT32_MAX;
constexpr int wait_timeout=-1, wait_failed=-2;
EventHandle make_event(bool manual_reset=false, bool signaled=false) noexcept;
void signal(const EventHandle& event);
void reset_event(const EventHandle& event);
void close_event(const EventHandle& event);
// Returns the lowest signaled index, or wait_timeout/wait_failed. Auto-reset
// signals coalesce and are consumed by one waiter; manual-reset signals persist.
int wait_any(std::initializer_list<EventHandle> events,uint32_t timeout=infinite);
inline int wait(const EventHandle& event,uint32_t timeout=infinite) { return wait_any({event},timeout); }
class EventOwner final {
    EventHandle event_;
public:
    EventOwner()=default;
    explicit EventOwner(EventHandle event):event_(std::move(event)) {}
    ~EventOwner() { reset(); }
    EventOwner(const EventOwner&)=delete;
    EventOwner& operator=(const EventOwner&)=delete;
    EventHandle get() const { return event_; }
    explicit operator bool() const { return bool(event_); }
    void reset(EventHandle event={}) { close_event(event_); event_=std::move(event); }
};
enum class Priority { normal, low, time_critical };
void set_current_priority(Priority priority) noexcept;
class Thread final {
    std::thread thread_;
public:
    Thread()=default;
    ~Thread() { reset(); }
    Thread(const Thread&)=delete;
    Thread& operator=(const Thread&)=delete;
    explicit operator bool() const noexcept { return thread_.joinable(); }
    // Callers wake/stop workers before joining, including partial startup.
    void reset() { if (thread_.joinable()) thread_.join(); }
    template<class Function> bool start(Function function,void* argument=nullptr,Priority priority=Priority::normal) noexcept {
        if (thread_.joinable()) return false;
        try { thread_=std::thread([=] { set_current_priority(priority); function(argument); }); }
        catch (...) { return false; }
        return true;
    }
};
class Lock final {
    std::recursive_mutex* mutex_;
public:
    explicit Lock(std::recursive_mutex* mutex):mutex_(mutex) { if(mutex_) mutex_->lock(); }
    ~Lock() { if(mutex_) mutex_->unlock(); }
    Lock(const Lock&)=delete;
    Lock& operator=(const Lock&)=delete;
};
}
