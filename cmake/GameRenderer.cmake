# The SDL GPU game owns its source targets directly. No comparison target clones.
target_link_libraries(kinoko_native_methods PUBLIC kinoko_game_graphics)
target_link_libraries(kinoko_modern_gpu PRIVATE kinoko_gpu kinoko_platform kinoko_audio_output)
add_dependencies(kinoko_modern_gpu kinoko_gpu_shaders)
# These independent fixtures compile renderer consumers and now need the actual
# project graphics implementation rather than SDK inline COM dispatch.
target_link_libraries(kinoko_map_chip_cache_contract PRIVATE kinoko_native_methods)
target_link_libraries(kinoko_application_contract PRIVATE kinoko_native_methods)


# Guard every game/library/contract target, not only the renderer. SDL carries
# the same guard in ModernPlatform.cmake. Portable libraries need no Windows SDK.
get_property(game_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS game_targets)
    get_target_property(kind ${target} TYPE)
    if(kind MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
        target_include_directories(${target} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/no-d3d9")
    endif()
endforeach()
