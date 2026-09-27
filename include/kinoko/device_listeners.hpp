#pragma once
#include "kinoko/graphics_runtime.hpp"
#include <algorithm>
#include <list>
#include <stdexcept>

namespace kinoko::graphics {
// The caller supplies the graphics lock. Registration order and duplicate
// suppression match the original list; only nodes, not listeners, are owned.
class DeviceListeners {
    std::list<KinokoDeviceListener*> entries_;
public:
    void clear() { entries_.clear(); }
    int add(KinokoDeviceListener* listener) {
        if (std::find(entries_.begin(), entries_.end(), listener) != entries_.end()) return 0;
        if (entries_.size() == 0x3ffffffeu) throw std::length_error("list<T> too long");
        entries_.push_back(listener);
        return 1;
    }
    void remove(KinokoDeviceListener* listener) {
        const auto found = std::find(entries_.begin(), entries_.end(), listener);
        if (found != entries_.end()) entries_.erase(found);
    }
    void notify(KinokoDeviceEvent event) {
        for (auto* listener : entries_) {
            const auto callback = event == KINOKO_DEVICE_BEFORE_RESET
                ? listener->before_reset : listener->after_reset;
            callback(listener->context);
        }
    }
};
}
