#pragma once
#include "kinoko/camera.h"
#include "kinoko/owned_script_object.h"
#include "kinoko/native_record_view.hpp"
#include <array>
#include <cstdint>
struct SQVM;
namespace kinoko::camera {
struct Bounds { float left, top, right, bottom; };
// Native-width slots own SqPlus external references, never SQObjectPtr.
using ScriptSlot = std::array<unsigned char,sizeof(KinokoOwnedObjectWords)>;
struct Record {
    alignas(void*) ScriptSlot script_object;
    SQVM *update_vm; // borrowed; copied without addref
    ScriptSlot update_environment, update_function;
    float x, y, center_x, center_y, offset_x, offset_y, width, height;
    Bounds bounds;
};
using View = native::RecordView<Record>;
static_assert(offsetof(Record,update_vm)==sizeof(ScriptSlot));
static_assert(offsetof(Record,update_environment)==sizeof(ScriptSlot)+sizeof(void*));
static_assert(offsetof(Record,update_function)==2*sizeof(ScriptSlot)+sizeof(void*));
static_assert(offsetof(Record,x)==3*sizeof(ScriptSlot)+sizeof(void*));
#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(Record)==88);
static_assert(offsetof(Record,update_vm)==12 && offsetof(Record,update_function)==28);
static_assert(offsetof(Record,x)==40 && offsetof(Record,offset_x)==56);
static_assert(offsetof(Record,width)==64 && offsetof(Record,bounds)==72);
#endif
}
