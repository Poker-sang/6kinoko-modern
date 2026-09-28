#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/native_property_bridge.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/collision_records.hpp"
#include <type_traits>
static_assert(sizeof(kinoko_layout_class_pair)==sizeof(HSQOBJECT));
static_assert(sizeof(kinoko_layer_set_pair)==sizeof(HSQOBJECT));
static_assert(sizeof(kinoko::act::ScriptValueStorage)==sizeof(HSQOBJECT));
static_assert(sizeof(kinoko::map::ManagerRecord::script_object)==sizeof(kinoko::script::ObjectStorage));
static_assert(std::is_same_v<decltype(&kinoko_sqrat_call_integer0),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_sqrat_call_integer1),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_c2dlayout_get_float),SQFUNCTION>);
static_assert(std::is_same_v<decltype(&kinoko_map_get_chip_layout),SQFUNCTION>);
static_assert(offsetof(KinokoCollisionRecord,index)==2*sizeof(void*));
static_assert(offsetof(kinoko::map::LayerRecord,name)==offsetof(kinoko::act::LayerStorageRecord,name));
