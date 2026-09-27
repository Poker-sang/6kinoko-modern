option(KINOKO_BUILD_LEGACY_COMPARISON "Build the old game and comparison contracts by default" OFF)
# Capture the pre-modern root targets. OFF excludes these from ALL unless the
# GPU game needs them. Explicit comparison targets remain available.
get_property(kinoko_comparison_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
get_property(kinoko_comparison_tests DIRECTORY PROPERTY TESTS)
# Separate full game target: preserve the D3D9 comparison build and its contracts.
function(kinoko_clone_for_gpu original replacement)
    get_target_property(sources ${original} SOURCES)
    add_library(${replacement} STATIC ${sources})
    foreach(property INCLUDE_DIRECTORIES COMPILE_OPTIONS COMPILE_DEFINITIONS LINK_LIBRARIES INTERFACE_INCLUDE_DIRECTORIES INTERFACE_COMPILE_OPTIONS INTERFACE_COMPILE_DEFINITIONS INTERFACE_LINK_LIBRARIES)
        get_target_property(value ${original} ${property})
        if(value)
            string(REPLACE "kinoko_native_methods" "kinoko_native_methods_gpu" value "${value}")
            string(REPLACE "kinoko_upstream_bindings" "kinoko_upstream_bindings_gpu" value "${value}")
            set_property(TARGET ${replacement} PROPERTY ${property} "${value}")
        endif()
    endforeach()
    target_compile_definitions(${replacement} PRIVATE KINOKO_GPU_GAME)
endfunction()
kinoko_clone_for_gpu(kinoko_native_methods kinoko_native_methods_gpu)
kinoko_clone_for_gpu(kinoko_upstream_bindings kinoko_upstream_bindings_gpu)
kinoko_clone_for_gpu(kinoko_squirrel_cpp_vm kinoko_squirrel_cpp_vm_gpu)
target_sources(kinoko_native_methods_gpu PRIVATE src/platform/gpu_game_device.cpp)
target_link_libraries(kinoko_native_methods_gpu PRIVATE kinoko_gpu)
get_target_property(game_sources kinoko_retdec_rebuild SOURCES)
add_executable(kinoko_modern_gpu WIN32 ${game_sources})
foreach(property INCLUDE_DIRECTORIES COMPILE_OPTIONS COMPILE_DEFINITIONS LINK_OPTIONS)
    get_target_property(value kinoko_retdec_rebuild ${property})
    if(value)
        set_property(TARGET kinoko_modern_gpu PROPERTY ${property} "${value}")
    endif()
endforeach()
target_compile_definitions(kinoko_modern_gpu PRIVATE KINOKO_GPU_GAME)
target_link_libraries(kinoko_modern_gpu PRIVATE
    kinoko_squirrel_cpp_vm_gpu kinoko_audio_runtime kinoko_actor_collision
    kinoko_native_methods_gpu kinoko_legacy_abi kinoko_retdec_support kinoko_diagnostics
    kinoko_zlib kinoko_gpu kinoko_platform kinoko_audio_output
    user32 gdi32 winmm imm32 ole32 dbghelp)
add_dependencies(kinoko_modern_gpu kinoko_gpu_shaders)

add_executable(kinoko_gpu_resource_contract tests/gpu_resource_contract.cpp)
target_compile_definitions(kinoko_gpu_resource_contract PRIVATE KINOKO_GPU_GAME)
target_include_directories(kinoko_gpu_resource_contract PRIVATE include)
target_link_libraries(kinoko_gpu_resource_contract PRIVATE kinoko_native_methods_gpu kinoko_gpu kinoko_platform user32)
add_test(NAME gpu_resource_contract COMMAND kinoko_gpu_resource_contract)

# This is a build guard, not a replacement SDK header: any accidental modern
# include of d3d9.h fails immediately, including transitive SDK dependencies.
foreach(target kinoko_modern_gpu kinoko_native_methods_gpu kinoko_upstream_bindings_gpu kinoko_squirrel_cpp_vm_gpu kinoko_gpu_resource_contract)
    target_include_directories(${target} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/no-d3d9")
endforeach()

if(KINOKO_BUILD_LEGACY_COMPARISON)
    add_library(kinoko_graphics_vocabulary_contract OBJECT tests/graphics_vocabulary_contract.cpp)
    target_include_directories(kinoko_graphics_vocabulary_contract PRIVATE include)
else()
    foreach(target IN LISTS kinoko_comparison_targets)
        set_property(TARGET ${target} PROPERTY EXCLUDE_FROM_ALL TRUE)
    endforeach()
    if(kinoko_comparison_tests)
        set_tests_properties(${kinoko_comparison_tests} PROPERTIES DISABLED TRUE)
    endif()
endif()
