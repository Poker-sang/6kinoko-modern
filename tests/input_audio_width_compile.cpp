#include "kinoko/audio_records.hpp"
#include "kinoko/input_manager.h"
#include "kinoko/squirrel_host_object.hpp"
#include <type_traits>
// Compile-only ABI checks: game/file integers do not follow pointer width.
static_assert(sizeof(KinokoInputAssignment) == 68);
static_assert(sizeof(KinokoInputState) == 96);
static_assert(sizeof(KinokoInputPublishedState) == 76);
static_assert(sizeof(KinokoInputManager::script_object) == sizeof(kinoko::script::ObjectStorage));
static_assert(std::is_same_v<decltype(KinokoInputDeviceMethods::update), decltype(&kinoko_input_device_update)>);
static_assert(std::is_same_v<decltype(KinokoInputDeviceMethods::destroy), decltype(&kinoko_input_device_delete)>);
static_assert(offsetof(KinokoKeyTracker, shift) == 1024 + sizeof(void*) + 12);
static_assert(offsetof(KinokoInputCluster, active_device) == offsetof(KinokoInputCluster, devices) + sizeof(void*) + 16);
static_assert(sizeof(kinoko::audio::BufferRecord::successor) == sizeof(uint32_t));
static_assert(sizeof(kinoko::audio::BufferRecord::predecessor) == sizeof(uint32_t));

#include "kinoko/application_runtime.hpp"
