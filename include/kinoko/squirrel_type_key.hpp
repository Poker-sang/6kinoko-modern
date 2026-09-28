#pragma once
#include <squirrel.h>
#include <cstdint>
namespace kinoko::script {
// Script numbers and serialized values stay 32-bit. Internal type identity keys
// use userpointers at native width, preserving original integer keys on x86.
inline void push_type_key(HSQUIRRELVM vm, const void* type) {
#if INTPTR_MAX == INT32_MAX
    sq_pushinteger(vm,static_cast<SQInteger>(reinterpret_cast<intptr_t>(type)));
#else
    sq_pushuserpointer(vm,const_cast<void*>(type));
#endif
}
inline void* type_pointer(HSQUIRRELVM vm, HSQOBJECT table, const void* type) {
    const auto top=sq_gettop(vm); void* result=nullptr;
    sq_pushobject(vm,table); push_type_key(vm,type);
    if(SQ_SUCCEEDED(sq_get(vm,-2))) sq_getuserpointer(vm,-1,&result);
    sq_settop(vm,top); return result;
}
inline void set_type_pointer(HSQUIRRELVM vm, HSQOBJECT table, const void* type, void* value) {
    const auto top=sq_gettop(vm);
    sq_pushobject(vm,table); push_type_key(vm,type); sq_pushuserpointer(vm,value);
    sq_rawset(vm,-3); sq_settop(vm,top);
}
inline void set_type_name(HSQUIRRELVM vm, HSQOBJECT table, const void* type, const char* name) {
    const auto top=sq_gettop(vm);
    sq_pushobject(vm,table); push_type_key(vm,type); sq_pushstring(vm,name,-1);
    sq_rawset(vm,-3); sq_settop(vm,top);
}
}
