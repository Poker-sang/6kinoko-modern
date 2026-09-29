# The renderer API is portable. Windows preview shader compilation is separate.
add_library(kinoko_gpu STATIC src/platform/sdl_gpu_renderer.cpp src/platform/gpu_vertices.cpp)
target_include_directories(kinoko_gpu PUBLIC include)
target_link_libraries(kinoko_gpu PUBLIC SDL3::SDL3-static)
target_compile_features(kinoko_gpu PUBLIC cxx_std_17)
add_executable(kinoko_gpu_vertices_contract tests/gpu_vertices_contract.cpp)
target_link_libraries(kinoko_gpu_vertices_contract PRIVATE kinoko_gpu)
add_test(NAME gpu_vertices_contract COMMAND kinoko_gpu_vertices_contract)
add_executable(kinoko_gpu_transfer_contract tests/gpu_transfer_contract.cpp)
target_link_libraries(kinoko_gpu_transfer_contract PRIVATE SDL3::SDL3-static)
target_compile_features(kinoko_gpu_transfer_contract PRIVATE cxx_std_17)
# Explicit/manual GPU contract: compile in CI, but do not require a GPU during
# ordinary CTest execution. Run this executable on the target machine.
if(WIN32)
    set(KINOKO_GPU_SHADER_SDK "10.0.22621.0" CACHE STRING "Pinned Windows SDK shader compiler version")
    set(KINOKO_FXC "C:/Program Files (x86)/Windows Kits/10/bin/${KINOKO_GPU_SHADER_SDK}/x64/fxc.exe" CACHE FILEPATH "Offline shader compiler (host x64)")
    if(NOT EXISTS "${KINOKO_FXC}")
        message(FATAL_ERROR "Pinned FXC not found: ${KINOKO_FXC}. Set KINOKO_FXC explicitly to an installed compiler; overrides are recorded.")
    endif()
    file(SHA256 "${KINOKO_FXC}" fxc_sha256)
    file(WRITE "${CMAKE_BINARY_DIR}/gpu-shader-toolchain.txt" "sdk=${KINOKO_GPU_SHADER_SDK}\ncompiler=${KINOKO_FXC}\nsha256=${fxc_sha256}\nprofiles=vs_5_1,ps_5_1\nflags=/nologo /Ges /O3 /E main\n")
    set(shader_dir "${KINOKO_RUNTIME_DIR}/shaders")
    foreach(stage vert frag)
        if(stage STREQUAL "vert")
            set(profile vs_5_1)
        else()
            set(profile ps_5_1)
        endif()
        add_custom_command(OUTPUT "${shader_dir}/sprite.${stage}.dxbc"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${shader_dir}"
            COMMAND "${KINOKO_FXC}" /nologo /Ges /O3 /E main /T ${profile}
                /Fo "${shader_dir}/sprite.${stage}.dxbc" "${CMAKE_SOURCE_DIR}/shaders/sprite.${stage}.hlsl"
            DEPENDS "${CMAKE_SOURCE_DIR}/shaders/sprite.${stage}.hlsl"
            VERBATIM)
    endforeach()
    add_custom_target(kinoko_gpu_shaders DEPENDS "${shader_dir}/sprite.vert.dxbc" "${shader_dir}/sprite.frag.dxbc")
    add_executable(kinoko_gpu_preview tools/gpu_preview.cpp)
    target_link_libraries(kinoko_gpu_preview PRIVATE kinoko_gpu)
    add_dependencies(kinoko_gpu_preview kinoko_gpu_shaders)
endif()

add_executable(kinoko_matrix_math_contract tests/matrix_math_contract.cpp)
target_include_directories(kinoko_matrix_math_contract PRIVATE include)
add_test(NAME matrix_math_contract COMMAND kinoko_matrix_math_contract)

add_library(kinoko_game_graphics STATIC src/platform/gpu_game_device.cpp)
target_include_directories(kinoko_game_graphics PUBLIC include)
target_link_libraries(kinoko_game_graphics PUBLIC kinoko_gpu kinoko_platform)
target_compile_features(kinoko_game_graphics PUBLIC cxx_std_17)
add_executable(kinoko_gpu_resource_contract tests/gpu_resource_contract.cpp)
target_link_libraries(kinoko_gpu_resource_contract PRIVATE kinoko_game_graphics)
add_test(NAME gpu_resource_contract COMMAND kinoko_gpu_resource_contract)

if(NOT WIN32)
    find_program(KINOKO_GLSLANG glslangValidator REQUIRED)
    if(APPLE)
        find_program(KINOKO_SPIRV_CROSS spirv-cross REQUIRED)
    endif()
    set(shader_dir "${KINOKO_RUNTIME_DIR}/shaders")
    set(shader_outputs)
    foreach(stage vert frag)
        set(spv "${shader_dir}/sprite.${stage}.spv")
        add_custom_command(OUTPUT "${spv}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${shader_dir}"
            COMMAND "${KINOKO_GLSLANG}" -V -S ${stage} -o "${spv}" "${CMAKE_SOURCE_DIR}/shaders/sprite.${stage}.glsl"
            DEPENDS "${CMAKE_SOURCE_DIR}/shaders/sprite.${stage}.glsl" VERBATIM)
        if(APPLE)
            set(msl "${shader_dir}/sprite.${stage}.msl")
            add_custom_command(OUTPUT "${msl}"
                COMMAND "${KINOKO_SPIRV_CROSS}" "${spv}" --msl --msl-version 20100 --output "${msl}"
                DEPENDS "${spv}" VERBATIM)
            list(APPEND shader_outputs "${msl}")
        else()
            list(APPEND shader_outputs "${spv}")
        endif()
    endforeach()
    add_custom_target(kinoko_gpu_shaders DEPENDS ${shader_outputs})
endif()
add_custom_target(kinoko_font_assets
    COMMAND ${CMAKE_COMMAND} -E make_directory "${KINOKO_RUNTIME_DIR}/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/noto/NotoSansCJKjp-Regular.otf" "${KINOKO_RUNTIME_DIR}/fonts/NotoSansCJKjp-Regular.otf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/noto/LICENSE" "${KINOKO_RUNTIME_DIR}/fonts/LICENSE"
    VERBATIM)
