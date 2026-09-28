#pragma once
#include "kinoko/actor_pool.h"
#include <cstring>
#include "kinoko/method_entry.hpp"

namespace kinoko::actor {
// Preserve the original virtual interface, including ABI fixture pools.
struct PoolMethods {
    int32_t (KINOKO_METHOD_ENTRY *destroy)(KinokoActorPool *,void*,unsigned char);
    KinokoActor *(KINOKO_METHOD_ENTRY *acquire)(KinokoActorPool *,void*,uint32_t *);
    int32_t (KINOKO_METHOD_ENTRY *retire)(KinokoActorPool *,void*,uint32_t);
};
inline PoolMethods pool_methods(KinokoActorPool *pool) {
    const PoolMethods *table;
    std::memcpy(&table,pool,sizeof(table));
    return *table;
}
}
