#include "kinoko/boost_hash.h"
#include "kinoko/serialized_hash.hpp"

extern "C" int32_t kinoko_boost_hash_range(const char* begin,const char* end) {
    return static_cast<int32_t>(kinoko::format::serialized_hash(begin,end));
}
