#pragma once
#include "kinoko/method_entry.hpp"
#include <mutex>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct KinokoCriticalSection KinokoCriticalSection;
typedef struct KinokoCriticalSectionMethods {
    KinokoCriticalSection* (KINOKO_METHOD_ENTRY *destroy)(KinokoCriticalSection*, void*, unsigned);
} KinokoCriticalSectionMethods;
/* Native owner: allocated separately because reconstructed records use raw storage. */
struct KinokoCriticalSection {
    const KinokoCriticalSectionMethods* methods;
    std::recursive_mutex* native;
};
extern const KinokoCriticalSectionMethods kinoko_critical_section_methods;
KinokoCriticalSection* kinoko_critical_section_construct(KinokoCriticalSection* self);
void kinoko_critical_section_destruct(KinokoCriticalSection* self);
KinokoCriticalSection* KINOKO_METHOD_ENTRY kinoko_critical_section_delete(
    KinokoCriticalSection* self, void* unused_edx, unsigned flags);
extern KinokoCriticalSection kinoko_graphics_lock;
#ifdef __cplusplus
}
#endif
