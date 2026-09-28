#include "kinoko/critical_section.h"
#pragma once

namespace kinoko::graphics {
// Original recursive lock; a successful BeginScene deliberately holds it
// until EndScene, so those two entry points use explicit acquisition/release.
class Lock final {
public:
    Lock() { kinoko_graphics_lock.native->lock(); }
    ~Lock() { kinoko_graphics_lock.native->unlock(); }
    Lock(const Lock&)=delete;
    Lock& operator=(const Lock&)=delete;
};
}
