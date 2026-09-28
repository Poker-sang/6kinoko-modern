#pragma once
#include "kinoko/act_types.h"
#include "kinoko/method_entry.hpp"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>

// 466100 publishes three independently allocated objects; 465F70 destroys
// holder, document (virtual deleting destructor), runtime, then this record.
// A schema only, never a C++ object overlaid on a C allocation/test fixture.
struct KinokoStageOwner {
    KinokoActDocument *document;
    KinokoActSourceHolder *holder;
    KinokoActRuntime *runtime;
};
struct KinokoActSourceHolder { KinokoActDocument *document; };
namespace kinoko::stage {
using OwnerRecord = KinokoStageOwner;
using SourceHolderRecord = KinokoActSourceHolder;
using DeleteDocument = kinoko::method::Entry<int32_t,int32_t>;
struct DocumentVirtuals {
    std::array<void *, 4> preceding_methods;
    DeleteDocument deleting_destructor;
};
struct DocumentPrefix { const DocumentVirtuals *vtable; };
using OwnerView = kinoko::native::RecordView<OwnerRecord>;
static_assert(sizeof(OwnerRecord) == 3*sizeof(void*));
static_assert(offsetof(OwnerRecord, holder) == sizeof(void*));
static_assert(offsetof(OwnerRecord, runtime) == 2*sizeof(void*));
static_assert(sizeof(SourceHolderRecord) == sizeof(void*));
static_assert(offsetof(DocumentVirtuals, deleting_destructor) == 4*sizeof(void*));
}
