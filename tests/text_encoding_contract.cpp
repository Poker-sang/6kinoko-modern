#include "kinoko/text_encoding.hpp"
#include "kinoko/runtime_util.hpp"
int main(){
    using namespace kinoko::text;
    const char japanese[]={char(0x93),char(0xfa),char(0x96),char(0x7b),0};
    if(codepoint(japanese)!=0x65e5||codepoint(japanese+2)!=0x672c)return 1;
    const char halfwidth[]={char(0xb6),0};
    if(codepoint(halfwidth)!=0xff76||next(halfwidth)!=halfwidth+1)return 2;
    if(next(japanese)!=japanese+2||previous(japanese,japanese+4)!=japanese+2)return 3;
    const char truncated[]={char(0x81),0};
    if(next(truncated)!=truncated+1||codepoint(truncated)!=0xfffd)return 4;
    if(codepoint("A")!=65||kinoko::compare_asset_names("Data/Stage","data/STAGE"))return 5;
    const char first[]={char(0x81),'A',0},second[]={char(0x81),'a',0};
    if(!kinoko::compare_asset_names(first,second))return 6; // DBCS trail bytes are not Latin letters.
    char copy[3]={char(0x81),'A',0};kinoko::lower_asset_name(copy,2);
    if(copy[1]!='A')return 7;
    return 0;
}
