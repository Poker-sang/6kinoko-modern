#pragma once
#include "kinoko/compiler_compat.h"
#include "kinoko/graphics_types.hpp"
#include "kinoko/text_encoding.hpp"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdint>
namespace kinoko {
inline int copy_bytes(void* dst,size_t capacity,const void* src,size_t count){if(!dst||(!src&&count))return EINVAL;if(count>capacity){std::memset(dst,0,capacity);return ERANGE;}if(count)std::memcpy(dst,src,count);return 0;}
inline int copy_string(char* dst,size_t capacity,const char* src){if(!dst||!capacity||!src)return EINVAL;const auto n=std::strlen(src);if(n>=capacity){dst[0]=0;return ERANGE;}std::memcpy(dst,src,n+1);return 0;}
inline int append_string(char* dst,size_t capacity,const char* src){if(!dst||!src)return EINVAL;size_t n=0;while(n<capacity&&dst[n])++n;if(n==capacity)return ERANGE;return copy_string(dst+n,capacity-n,src);}
inline int compare_asset_names(const char* a,const char* b){
    bool trail_a=false,trail_b=false;
    for(;;){
        auto x=static_cast<unsigned char>(*a++),y=static_cast<unsigned char>(*b++);
        const bool next_a=!trail_a&&text::lead_byte(x),next_b=!trail_b&&text::lead_byte(y);
        if(!trail_a&&x>='A'&&x<='Z')x+=32;
        if(!trail_b&&y>='A'&&y<='Z')y+=32;
        if(x!=y||!x||!y)return int(x)-int(y);
        trail_a=next_a;trail_b=next_b;
    }
}
inline void lower_asset_name(char* text,size_t length){for(size_t i=0;i<length;++i){auto c=static_cast<unsigned char>(text[i]);if(text::lead_byte(c)&&i+1<length){++i;continue;}if(c>='A'&&c<='Z')text[i]=static_cast<char>(c+32);}}
}
