#include "kinoko/squirrel_host_object.hpp"
#include <type_traits>

using kinoko::script::ObjectStorage;
using kinoko::script::ObjectView;
static_assert(offsetof(ObjectStorage, value) == sizeof(void*));
static_assert(std::is_same_v<decltype(std::declval<ObjectView>().payload_data()), void*>);
static_assert(sizeof(ObjectStorage) >= sizeof(void*) + sizeof(HSQOBJECT));

void* kinoko_squirrel_payload_width_probe(ObjectStorage* storage) {
    return ObjectView(storage).payload_data();
}
