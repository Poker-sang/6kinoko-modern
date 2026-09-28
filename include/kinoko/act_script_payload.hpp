#pragma once
#include "kinoko/act_script_storage.hpp"
namespace kinoko::act {
// The loader and destructor share the complete native script schema.
using ScriptPayloadRecord = ScriptStorageRecord;
using ScriptPayloadView = ScriptStorageView;
}
