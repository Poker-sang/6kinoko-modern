#include "kinoko/method_entry.hpp"
#include <cstdint>
#include <cstdlib>
#include <array>
namespace {
void require(bool value) { if(!value) std::abort(); }
struct Receiver { int calls=0; };
struct Payload { void* object; std::uintptr_t identity; double value; };
void KINOKO_METHOD_ENTRY touch(void* self,void* reserved,int32_t argument) {
    require(!reserved && argument==-19);++static_cast<Receiver*>(self)->calls;
}
int32_t KINOKO_METHOD_ENTRY move(void* self,void* reserved,float x,float y) {
    require(!reserved && x==1.25f && y==-2.5f);++static_cast<Receiver*>(self)->calls;return 0x100;
}
int32_t KINOKO_METHOD_ENTRY rectangle(void* self,void* reserved,float l,float t,float r,float b) {
    require(!reserved && l==-1.5f && t==2.25f && r==30.5f && b==-40.75f);
    ++static_cast<Receiver*>(self)->calls;return 0x101;
}
void* KINOKO_METHOD_ENTRY mixed(void* self,void* reserved,int32_t x,float alpha,
    void* resource,float y,int32_t count) {
    require(!reserved && x==-11 && alpha==0.375f && y==-3.5f && count==17);
    ++static_cast<Receiver*>(self)->calls;return resource;
}
std::uintptr_t KINOKO_METHOD_ENTRY owned(void* self,void* reserved,Payload value) {
    require(!reserved && value.object==self && value.value==-17.25);
    ++static_cast<Receiver*>(self)->calls;return value.identity;
}
}
int main() {
    Receiver object;
    constexpr std::uintptr_t identity=sizeof(void*)==8 ? UINT64_C(0x123456789abcdef0) : UINT64_C(0x76543210);
    auto* pointer=reinterpret_cast<void*>(identity); // Compare only, never dereference.
    for(int i=0;i<10000;++i) {
        kinoko::method::invoke<void>(&object,reinterpret_cast<void*>(touch),int32_t{-19});
        require(kinoko::method::invoke<int32_t>(&object,reinterpret_cast<void*>(move),1.25f,-2.5f)==0x100);
        require(kinoko::method::invoke<int32_t>(&object,reinterpret_cast<void*>(rectangle),-1.5f,2.25f,30.5f,-40.75f)==0x101);
        require(kinoko::method::invoke<void*>(&object,reinterpret_cast<void*>(mixed),int32_t{-11},0.375f,pointer,-3.5f,int32_t{17})==pointer);
        require(kinoko::method::invoke<std::uintptr_t>(&object,reinterpret_cast<void*>(owned),Payload{&object,identity,-17.25})==identity);
    }
    require(object.calls==50000);
    return 0;
}
