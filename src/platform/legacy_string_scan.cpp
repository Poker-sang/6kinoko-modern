#include "kinoko/legacy_string.h"
#include <cstring>
extern "C" uint32_t kinoko_safe_c_string_length(const char* source) {
    if(!source)return 0;
    // Audio resource names are terminated borrowed strings. Retain the length cap.
    for(uint32_t n=0;n<0x100000u;++n) if(!source[n])return n;
    return 0;
}
extern "C" void* kinoko_string_assign_cstr(void* object,const char* source) {
    return kinoko_string_assign_n(object,source,kinoko_safe_c_string_length(source));
}
