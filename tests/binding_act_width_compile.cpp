#include "kinoko/act_layer_storage.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_binding.h"
#include <type_traits>
using namespace kinoko::act;
static_assert(sizeof(ScriptValueStorage)==sizeof(HSQOBJECT));
static_assert(alignof(ScriptStorageRecord)>=alignof(HSQOBJECT));
static_assert(offsetof(LayerStorageRecord,script_object)%alignof(void*)==0);
static_assert(sizeof(DocumentRecord::script)==sizeof(ScriptStorageRecord));
static_assert(std::is_same_v<decltype(&kinoko_sqplus_object_method),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_sqplus_instance_get),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_sqplus_table_set),SQFUNCTION>);
