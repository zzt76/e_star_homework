set(HOMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework")

# 在此处添加文件
set(HOMEWORK_SOURCES
        ${HOMEWORK_DIR}/homework.cpp
        ${HOMEWORK_DIR}/bgfx_logo.h
        )

add_executable(homework ${HOMEWORK_SOURCES})
target_link_libraries(homework example-common)
target_include_directories(homework PRIVATE ${HOMEWORK_DIR})

add_custom_target(shaders
        ALL
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_cubes.sc ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_cubes.sc
        )
add_custom_command(TARGET shaders
        PRE_BUILD
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_cubes.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/vs_cubes.bin --type v --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_cubes.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/fs_cubes.bin --type f --platform windows
        )
add_dependencies(homework shaders)

# Special Visual Studio Flags
if (MSVC)
    target_compile_definitions(homework PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif ()
