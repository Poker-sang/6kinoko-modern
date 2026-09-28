#include "kinoko/compiler_compat.h"
#pragma once
#include <cstddef>
#include <cstring>

#if defined(_MSC_VER)
#define KINOKO_METHOD_ENTRY __fastcall
#elif defined(__i386__)
#define KINOKO_METHOD_ENTRY __attribute__((fastcall))
#else
#define KINOKO_METHOD_ENTRY
#endif

namespace kinoko::method {
// Reconstructed functions explicitly include the old unused EDX parameter.
// Keep that parameter at every pointer width. On x64 it is a real ABI argument;
// omitting it shifts integers/pointers AND changes floating-register positions.
// This adapter is for reconstructed entries, not arbitrary C++ member functions.
template<class Result,class... Args>
using Entry=Result (KINOKO_METHOD_ENTRY *)(void*,void*,Args...);
template<class Result,class... Args>
Result invoke(void* receiver,void* function,Args... args) {
    return reinterpret_cast<Entry<Result,Args...>>(function)(receiver,nullptr,args...);
}
template<class Fn> Fn entry(const void* object,std::size_t slot) noexcept {
    const unsigned char* table=nullptr;
    if(object) std::memcpy(&table,object,sizeof(table));
    Fn function=nullptr;
    static_assert(sizeof(function)==sizeof(void*));
    if(table) std::memcpy(&function,table+slot*sizeof(void*),sizeof(function));
    return function;
}
}
