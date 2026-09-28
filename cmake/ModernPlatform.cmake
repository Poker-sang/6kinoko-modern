# This module has no dependency on the legacy Win32/x86 runtime.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
# SDL GPU is the game renderer. Never build SDL's unrelated D3D9 renderer.
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL3-3.4.16 EXCLUDE_FROM_ALL)
target_include_directories(SDL3-static BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/no-d3d9")
add_library(kinoko_platform STATIC src/platform/sdl_platform.cpp)
target_include_directories(kinoko_platform PUBLIC include)
target_link_libraries(kinoko_platform PUBLIC SDL3::SDL3-static)
target_compile_features(kinoko_platform PUBLIC cxx_std_17)
add_executable(kinoko_platform_contract tests/platform_contract.cpp)
target_link_libraries(kinoko_platform_contract PRIVATE kinoko_platform)
enable_testing()
add_test(NAME platform_contract COMMAND kinoko_platform_contract)
set_tests_properties(platform_contract PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 30)

add_library(kinoko_audio_output STATIC src/platform/audio_ring.cpp src/platform/sdl_audio_output.cpp)
target_include_directories(kinoko_audio_output PUBLIC include)
target_link_libraries(kinoko_audio_output PUBLIC SDL3::SDL3-static)
target_compile_features(kinoko_audio_output PUBLIC cxx_std_17)
add_executable(kinoko_audio_output_contract tests/audio_output_contract.cpp)
target_link_libraries(kinoko_audio_output_contract PRIVATE kinoko_audio_output)
add_test(NAME audio_output_contract COMMAND kinoko_audio_output_contract)
set_tests_properties(audio_output_contract PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy" TIMEOUT 30)

# The semantic blend interface has no SDL/Windows dependency.
add_library(kinoko_render_state INTERFACE)
target_include_directories(kinoko_render_state INTERFACE include)
target_compile_features(kinoko_render_state INTERFACE cxx_std_17)
add_executable(kinoko_render_blend_contract tests/render_blend_contract.cpp)
target_link_libraries(kinoko_render_blend_contract PRIVATE kinoko_render_state)
add_test(NAME render_blend_contract COMMAND kinoko_render_blend_contract)

add_executable(kinoko_render_resources_contract tests/render_resources_contract.cpp)
target_link_libraries(kinoko_render_resources_contract PRIVATE kinoko_render_state)
add_test(NAME render_resources_contract COMMAND kinoko_render_resources_contract)

include(${CMAKE_CURRENT_LIST_DIR}/ModernGpu.cmake)

# Runtime-only graphics records and explicit listener dispatch compile on every
# portable target, including Windows x64, without original fixed-layout headers.
add_executable(kinoko_graphics_runtime_contract tests/graphics_runtime_contract.cpp)
target_include_directories(kinoko_graphics_runtime_contract PRIVATE include "${CMAKE_CURRENT_SOURCE_DIR}/cmake/no-d3d9")
target_compile_features(kinoko_graphics_runtime_contract PRIVATE cxx_std_17)
add_test(NAME graphics_runtime_contract COMMAND kinoko_graphics_runtime_contract)

# Font ownership is native-width; the ACT configuration bridge remains x86.
add_executable(kinoko_font_runtime_contract tests/font_runtime_contract.cpp)
target_include_directories(kinoko_font_runtime_contract PRIVATE include)
target_compile_features(kinoko_font_runtime_contract PRIVATE cxx_std_17)
add_test(NAME font_runtime_contract COMMAND kinoko_font_runtime_contract)
add_executable(kinoko_act_texture_runtime_contract tests/act_texture_runtime_contract.cpp
    src/reconstructed/legacy_string.cpp)
target_include_directories(kinoko_act_texture_runtime_contract PRIVATE include)
target_compile_features(kinoko_act_texture_runtime_contract PRIVATE cxx_std_17)
add_test(NAME act_texture_runtime_contract COMMAND kinoko_act_texture_runtime_contract)
add_executable(kinoko_act_chip_runtime_contract tests/act_chip_runtime_contract.cpp
    src/reconstructed/legacy_string.cpp)
target_include_directories(kinoko_act_chip_runtime_contract PRIVATE include)
target_compile_features(kinoko_act_chip_runtime_contract PRIVATE cxx_std_17)
add_test(NAME act_chip_runtime_contract COMMAND kinoko_act_chip_runtime_contract)
add_executable(kinoko_act_method_dispatch_contract tests/act_method_dispatch_contract.cpp)
target_include_directories(kinoko_act_method_dispatch_contract PRIVATE include)
target_compile_features(kinoko_act_method_dispatch_contract PRIVATE cxx_std_17)
add_test(NAME act_method_dispatch_contract COMMAND kinoko_act_method_dispatch_contract)
add_executable(kinoko_mesh_resource_contract tests/mesh_resource_contract.cpp
    src/reconstructed/legacy_string.cpp)
target_include_directories(kinoko_mesh_resource_contract PRIVATE include)
target_compile_features(kinoko_mesh_resource_contract PRIVATE cxx_std_17)
add_test(NAME mesh_resource_contract COMMAND kinoko_mesh_resource_contract)
add_executable(kinoko_mesh_decoder_contract tests/mesh_decoder_contract.cpp
    src/reconstructed/mesh_model.cpp)
target_include_directories(kinoko_mesh_decoder_contract PRIVATE include)
target_compile_features(kinoko_mesh_decoder_contract PRIVATE cxx_std_17)
add_test(NAME mesh_decoder_contract COMMAND kinoko_mesh_decoder_contract)
add_executable(kinoko_legacy_string_width_contract tests/legacy_string_width_contract.cpp)
target_include_directories(kinoko_legacy_string_width_contract PRIVATE include)
target_compile_features(kinoko_legacy_string_width_contract PRIVATE cxx_std_17)
add_test(NAME legacy_string_width_contract COMMAND kinoko_legacy_string_width_contract)
if(WIN32)
    add_library(kinoko_font_raster_compile OBJECT src/reconstructed/string_font.cpp)
    target_include_directories(kinoko_font_raster_compile PRIVATE include "${CMAKE_CURRENT_SOURCE_DIR}/cmake/no-d3d9")
    target_compile_features(kinoko_font_raster_compile PRIVATE cxx_std_17)
    add_library(kinoko_squirrel_object_width_compile OBJECT tests/squirrel_object_width_compile.cpp)
    target_include_directories(kinoko_squirrel_object_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include")
    target_compile_features(kinoko_squirrel_object_width_compile PRIVATE cxx_std_17)
    add_library(kinoko_legacy_string_width_compile OBJECT src/reconstructed/legacy_string.cpp)
    target_include_directories(kinoko_legacy_string_width_compile PRIVATE include)
    target_compile_features(kinoko_legacy_string_width_compile PRIVATE cxx_std_17)
endif()

# Native text ownership and clone state compile on x86/x64 without Win32.
add_executable(kinoko_string_runtime_contract tests/string_runtime_contract.cpp
    src/reconstructed/legacy_string.cpp)
target_include_directories(kinoko_string_runtime_contract PRIVATE include)
target_compile_features(kinoko_string_runtime_contract PRIVATE cxx_std_17)
add_test(NAME string_runtime_contract COMMAND kinoko_string_runtime_contract)

add_executable(kinoko_method_entry_contract tests/method_entry_contract.cpp)
target_include_directories(kinoko_method_entry_contract PRIVATE include)
target_compile_features(kinoko_method_entry_contract PRIVATE cxx_std_17)
add_test(NAME method_entry_contract COMMAND kinoko_method_entry_contract)
if(WIN32)
    add_library(kinoko_native_calls_width_compile OBJECT src/squirrel/squirrel_native_calls.cpp)
    target_include_directories(kinoko_native_calls_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include")
    target_compile_features(kinoko_native_calls_width_compile PRIVATE cxx_std_17)
endif()

add_executable(kinoko_serialized_hash_contract tests/serialized_hash_contract.cpp
    src/reconstructed/boost_hash.cpp)
target_include_directories(kinoko_serialized_hash_contract PRIVATE include)
target_compile_features(kinoko_serialized_hash_contract PRIVATE cxx_std_17)
add_test(NAME serialized_hash_contract COMMAND kinoko_serialized_hash_contract)
if(WIN32)
    add_library(kinoko_windows_services_width_compile OBJECT
        src/platform/diagnostics.cpp src/reconstructed/critical_section.cpp)
    target_include_directories(kinoko_windows_services_width_compile PRIVATE include)
    target_compile_features(kinoko_windows_services_width_compile PRIVATE cxx_std_17)
endif()

# Compile the real actor/camera and container implementations at native width.
# This object target deliberately does not claim that the full host can link yet.
if(WIN32)
    add_library(kinoko_actor_native_width_compile OBJECT
        src/reconstructed/actor_pool.cpp src/reconstructed/actor_manager.cpp
        src/reconstructed/actor_owner_list.cpp src/reconstructed/actor_priority.cpp
        src/reconstructed/animation_storage.cpp src/reconstructed/actor_animation.cpp
        src/reconstructed/actor_render.cpp src/reconstructed/camera_runtime.cpp
        src/reconstructed/camera_projection.cpp src/reconstructed/script_callbacks.cpp
        src/reconstructed/integer_map.cpp src/reconstructed/integer_vector.cpp
        src/reconstructed/native_buffer.cpp src/reconstructed/native_control.cpp
        src/squirrel/actor_state.cpp src/squirrel/actor_lifecycle.cpp
        src/squirrel/actor_initialization.cpp)
    target_include_directories(kinoko_actor_native_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include")
    target_compile_features(kinoko_actor_native_width_compile PRIVATE cxx_std_17)
endif()

if(WIN32)
    add_executable(kinoko_native_ownership_width_contract
        tests/native_ownership_width_contract.cpp src/reconstructed/native_control.cpp
        src/reconstructed/boost_control.cpp src/reconstructed/integer_map.cpp
        src/reconstructed/integer_vector.cpp)
    target_include_directories(kinoko_native_ownership_width_contract PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost-1.44.0")
    target_compile_features(kinoko_native_ownership_width_contract PRIVATE cxx_std_17)
    add_test(NAME native_ownership_width_contract COMMAND kinoko_native_ownership_width_contract)
endif()

if(WIN32)
    add_library(kinoko_binding_act_width_compile OBJECT
        tests/binding_act_width_compile.cpp
        src/squirrel/squirrel_class_binding.cpp src/squirrel/squirrel_native_variables.cpp
        src/squirrel/squirrel_native_arguments.cpp src/squirrel/squirrel_method_dispatch.cpp
        src/squirrel/squirrel_host_compat.cpp src/squirrel/squirrel_vm_bootstrap.cpp
        src/squirrel/sqrat_object_bridge.cpp src/squirrel/squirrel_game_objects.cpp
        src/squirrel/native_property_bridge.cpp src/squirrel/actor_registration.cpp
        src/squirrel/upstream_sqplus.cpp src/squirrel/upstream_sqplus_scalars.cpp
        src/squirrel/squirrel_type_registry.cpp
        third_party/sqplus-20080713/sqplus/SqPlus.cpp
        third_party/sqplus-20080713/sqplus/SquirrelObject.cpp
        src/reconstructed/act_script_lifecycle.cpp src/reconstructed/act_script_io.cpp
        src/reconstructed/act_layer_lifecycle.cpp src/reconstructed/act_layer_clone.cpp
        src/reconstructed/act_document_io.cpp src/reconstructed/act_document_clone.cpp
        src/reconstructed/act_layer_access.cpp src/reconstructed/act_frame_update.cpp
        src/reconstructed/act_runtime_lifecycle.cpp src/reconstructed/act_document_association.cpp
        src/squirrel/squirrel_source_gc.cpp src/squirrel/squirrel_value_bridge.cpp
        src/squirrel/global_registration.cpp src/squirrel/game_script_entries.cpp
        src/squirrel/squirrel_native_calls.cpp)
    target_include_directories(kinoko_binding_act_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/squirrel"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqplus-20080713/sqplus")
    target_compile_definitions(kinoko_binding_act_width_compile PRIVATE
        SQPLUS_HOST_OBJECT_ONLY WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_compile_features(kinoko_binding_act_width_compile PRIVATE cxx_std_17)
endif()

if(WIN32)
    add_library(kinoko_act_render_width_compile OBJECT
        src/reconstructed/act_layout.cpp src/reconstructed/act_layout_3d.cpp
        src/reconstructed/act_frame_render.cpp src/reconstructed/act_draw_storage.cpp
        src/reconstructed/act_clone.cpp src/reconstructed/act_array.cpp
        src/reconstructed/act_list.cpp src/reconstructed/act_resource.cpp
        src/reconstructed/act_serializable.cpp src/reconstructed/act_source.cpp
        src/reconstructed/act_lifetime.cpp)
    target_include_directories(kinoko_act_render_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include")
    target_compile_definitions(kinoko_act_render_width_compile PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_compile_features(kinoko_act_render_width_compile PRIVATE cxx_std_17)
endif()

if(WIN32)
    add_library(kinoko_act_map_width_compile OBJECT
        src/reconstructed/act_document.cpp src/reconstructed/act_texture_io.cpp
        src/reconstructed/map_activation.cpp src/reconstructed/map_chip_cache.cpp
        src/reconstructed/map_collision.cpp src/reconstructed/map_containers.cpp
        src/reconstructed/map_copy.cpp src/reconstructed/map_layers.cpp
        src/reconstructed/map_layout_lifetime.cpp src/reconstructed/map_loading.cpp
        src/reconstructed/map_manager.cpp src/reconstructed/map_render.cpp
        src/reconstructed/actor_collision.cpp src/reconstructed/collision_manager.cpp
        src/reconstructed/collision_queries.cpp
        src/squirrel/act_binding.cpp src/squirrel/native_property_bridge.cpp
        src/reconstructed/act_mesh.cpp src/squirrel/sqrat_object_bridge.cpp
        tests/act_map_width_compile.cpp)
    target_sources(kinoko_act_map_width_compile PRIVATE
        src/reconstructed/act_array.cpp
        src/reconstructed/act_clone.cpp
        src/reconstructed/act_document_association.cpp
        src/reconstructed/act_document_clone.cpp
        src/reconstructed/act_document_io.cpp
        src/reconstructed/act_document_resources.cpp
        src/reconstructed/act_draw_storage.cpp
        src/reconstructed/act_frame_render.cpp
        src/reconstructed/act_frame_update.cpp
        src/reconstructed/act_layer_access.cpp
        src/reconstructed/act_layer_clone.cpp
        src/reconstructed/act_layer_lifecycle.cpp
        src/reconstructed/act_layout.cpp
        src/reconstructed/act_layout_3d.cpp
        src/reconstructed/act_lifetime.cpp
        src/reconstructed/act_list.cpp
        src/reconstructed/act_map.cpp
        src/reconstructed/act_render_state.cpp
        src/reconstructed/act_resource.cpp
        src/reconstructed/act_runtime_lifecycle.cpp
        src/reconstructed/act_script_io.cpp
        src/reconstructed/act_script_lifecycle.cpp
        src/reconstructed/act_serializable.cpp
        src/reconstructed/act_source.cpp
        src/reconstructed/actor_motion.cpp
        src/reconstructed/string_layout_render.cpp
        src/reconstructed/string_render.cpp
        src/squirrel/camera_map_binding.cpp)
    target_include_directories(kinoko_act_map_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/squirrel")
    target_compile_definitions(kinoko_act_map_width_compile PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_compile_features(kinoko_act_map_width_compile PRIVATE cxx_std_17)
endif()

if(WIN32)
    # Compile real input publication and audio ownership, with native pointers.
    add_library(kinoko_input_audio_width_compile OBJECT
        src/reconstructed/audio_runtime.cpp
        src/reconstructed/physical_input.cpp
        src/reconstructed/input_aggregation.cpp
        src/reconstructed/input_copy.cpp
        src/reconstructed/input_frame.cpp
        src/reconstructed/input_keys.cpp
        src/reconstructed/input_runtime.cpp
        src/squirrel/input_registration.cpp
        src/reconstructed/application_runtime.cpp
        src/reconstructed/scene_queue.cpp
        src/reconstructed/game_runtime.cpp
        src/reconstructed/ime_input.cpp
        tests/input_audio_width_compile.cpp)
    target_include_directories(kinoko_input_audio_width_compile PRIVATE include
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/squirrel-2.2.2/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL3-3.4.16/include")
    target_compile_definitions(kinoko_input_audio_width_compile PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS KINOKO_RETDEC_DISABLE_TRACE=1)
    target_compile_features(kinoko_input_audio_width_compile PRIVATE cxx_std_17)
endif()

# OS files stay behind a native-width, Windows-header-free interface.
add_library(kinoko_file_service STATIC src/platform/file_service.cpp src/platform/directory_search.cpp)
target_include_directories(kinoko_file_service PUBLIC include)
target_link_libraries(kinoko_file_service PUBLIC SDL3::SDL3-static)
target_compile_features(kinoko_file_service PUBLIC cxx_std_17)
add_executable(kinoko_file_service_contract tests/file_service_contract.cpp)
target_link_libraries(kinoko_file_service_contract PRIVATE kinoko_file_service)
add_test(NAME file_service_contract COMMAND kinoko_file_service_contract)
# Compile the actual native reader/package consumer on every portable CI host.
add_library(kinoko_file_reader_compile OBJECT src/reconstructed/file_io.cpp)
target_include_directories(kinoko_file_reader_compile PRIVATE include)
target_compile_features(kinoko_file_reader_compile PRIVATE cxx_std_17)

find_package(Threads REQUIRED)
add_library(kinoko_runtime_services STATIC src/platform/runtime_sync.cpp src/platform/runtime_clock.cpp src/platform/runtime_paths.cpp)
target_include_directories(kinoko_runtime_services PUBLIC include)
target_link_libraries(kinoko_runtime_services PUBLIC SDL3::SDL3-static Threads::Threads)
if(WIN32)
    target_link_libraries(kinoko_runtime_services PRIVATE winmm)
endif()
target_compile_features(kinoko_runtime_services PUBLIC cxx_std_17)
add_executable(kinoko_runtime_services_contract tests/runtime_services_contract.cpp src/reconstructed/timer_events.cpp src/reconstructed/critical_section.cpp)
target_link_libraries(kinoko_runtime_services_contract PRIVATE kinoko_runtime_services)
add_test(NAME runtime_services_contract COMMAND kinoko_runtime_services_contract)

add_executable(kinoko_directory_search_contract tests/directory_search_contract.cpp)
target_link_libraries(kinoko_directory_search_contract PRIVATE kinoko_file_service)
add_test(NAME directory_search_contract COMMAND kinoko_directory_search_contract)
