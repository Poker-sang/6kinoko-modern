#include "kinoko/actor_records.hpp"
#include "kinoko/camera_records.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/native_control.h"
#include "kinoko/integer_map.h"
#include "kinoko/integer_vector.h"
#include "kinoko/method_entry.hpp"
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <stdexcept>
namespace {
void require(bool value) { if (!value) throw std::runtime_error("native ownership contract"); }
KinokoOwnedObjectWords KINOKO_METHOD_ENTRY echo(void*, void*, KinokoOwnedObjectWords value) { return value; }
}
int main() {
    // A deliberately high, never dereferenced pointer tests both payload words.
    const uintptr_t bits=sizeof(void*)==8 ? UINT64_C(0x1234567887654321) : UINT64_C(0x87654321);
    KinokoOwnedObjectWords input{reinterpret_cast<void*>(bits), 0, static_cast<intptr_t>(bits)};
    auto output=echo(nullptr,nullptr,input);
    kinoko::actor::ActorRecord actor{};
    std::memcpy(actor.script_object.data(), &output, sizeof output);
    KinokoOwnedObjectWords restored{};
    std::memcpy(&restored, actor.script_object.data(), sizeof restored);
    require(restored.vtable==input.vtable && restored.value==input.value);
    void *control=nullptr;
    void *allocation=std::malloc(sizeof(void*));
    require(allocation!=nullptr);
    kinoko_native_control_create(&control,allocation);
    require(control!=nullptr);
    kinoko_native_add_weak(control);
    KinokoNativeReference weak{allocation,control}, locked{};
    kinoko_native_weak_pair_lock(&weak,&locked);
    require(locked.allocation==allocation && locked.control==control);
    kinoko_native_release_strong(locked.control);
    KinokoNativeReference alias=weak;
    kinoko_native_weak_pair_lock(&alias,&alias);
    require(!alias.control && !alias.allocation);
    kinoko_native_release_strong(control);
    kinoko_native_weak_pair_lock(&weak,&locked);
    require(!locked.control && !locked.allocation);
    kinoko_native_release_weak(control);
    auto *map=kinoko_integer_map_create();
    auto *node=kinoko_integer_map_put(map,1,42);
    for(int i=2;i<1024;++i) kinoko_integer_map_put(map,i,i);
    require(kinoko_integer_map_find(map,1)==node && *node==42);
    kinoko_integer_map_destroy(map);
    KinokoIntegerVector vector{};
    kinoko_integer_vector_construct(&vector);
    for(int i=0;i<1024;++i) kinoko_integer_vector_append(&vector,i);
    require(kinoko_integer_vector_size(&vector)==1024 && kinoko_integer_vector_data(&vector)[1023]==1023);
    kinoko_integer_vector_destroy(&vector);
}
