#pragma once
#include "kinoko/act_types.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/act_method_dispatch.hpp"
#include <cstddef>

namespace kinoko::act {
// ISerializable::Dispose (+12) and CAct's deleting destructor (+16) are
// different contracts. Layout slot +16 is RTTI, NOT a deleting destructor.
template<class T> void dispose_owned(T *value) {
    SerializableMethods(value).dispose();
}
inline void delete_document(KinokoActDocument *value) {
    DocumentMethods(value).delete_object(1);
}
struct DocumentDeleter { void operator()(KinokoActDocument *p) const { delete_document(p); } };
template<class T> struct OwnedDeleter { void operator()(T *p) const { dispose_owned(p); } };
}
