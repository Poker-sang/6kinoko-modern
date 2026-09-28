#include "kinoko/text_encoding.hpp"
namespace kinoko::text {
uint32_t codepoint(const char* p){
    static const uint16_t mapping[65536]={
#include "cp932_table.inc"
    };
    if(!p||!*p)return 0;
    const auto lead=static_cast<unsigned char>(p[0]);
    return mapping[lead_byte(lead)&&p[1]?(lead<<8)|static_cast<unsigned char>(p[1]):lead];
}
}
