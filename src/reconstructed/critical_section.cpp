#include "kinoko/critical_section.h"
#include <cstddef>
#include <cstdlib>

// All consumers use the native field; this lock is never a serialized record.
static_assert(offsetof(KinokoCriticalSection, native) == sizeof(void*));
static_assert(sizeof(KinokoCriticalSection) == sizeof(void*)+sizeof(void*));
extern "C" const KinokoCriticalSectionMethods kinoko_critical_section_methods{
    kinoko_critical_section_delete
};
extern "C" KinokoCriticalSection* kinoko_critical_section_construct(KinokoCriticalSection* self) {
    self->methods = &kinoko_critical_section_methods;
    self->native = new std::recursive_mutex;
    return self;
}
extern "C" void kinoko_critical_section_destruct(KinokoCriticalSection* self) {
    self->methods = &kinoko_critical_section_methods;
    delete self->native;
    self->native = nullptr;
}
extern "C" KinokoCriticalSection* KINOKO_METHOD_ENTRY kinoko_critical_section_delete(
    KinokoCriticalSection* self, void*, unsigned flags) {
    kinoko_critical_section_destruct(self);
    // Host original operator-delete adapter uses the CRT allocation family.
    if (flags & 1u) std::free(self);
    return self; // Original scalar-deleting destructor returns its receiver.
}
