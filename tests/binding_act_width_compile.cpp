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

#include "kinoko/squirrel_native_calls.h"
#include "kinoko/game_script_api.h"
static_assert(std::is_same_v<decltype(&kinoko_native_void_entry),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_native_integer_pair_entry),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_script_create_actor_entry),SQFUNCTION>);
