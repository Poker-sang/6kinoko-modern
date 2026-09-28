#pragma once
#include <cstdint>
#include <cstddef>
namespace kinoko::text {
inline bool lead_byte(unsigned char c){return (c>=0x81&&c<=0x9f)||(c>=0xe0&&c<=0xfc);}
inline const char* next(const char* p){return !p||!*p?p:p+(lead_byte(static_cast<unsigned char>(*p))&&p[1]?2:1);}
inline const char* previous(const char* first,const char* end){const char* p=first;const char* last=first;while(p<end){last=p;p=next(p);}return last;}
uint32_t codepoint(const char* character);
}
