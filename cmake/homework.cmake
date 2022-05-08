set(HOMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework")

# 在此处添加文件
set(HOMEWORK_SOURCES
        ${HOMEWORK_DIR}/homework.cpp
        ${HOMEWORK_DIR}/mesh_producer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/3rdparty/FileBrowser/ImGuiFileBrowser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/3rdparty/FileBrowser/ImGuiFileBrowser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/3rdparty/FileBrowser/Dirent/dirent.h)

add_executable(homework ${HOMEWORK_SOURCES})
target_link_libraries(homework example-common)
target_include_directories(homework PRIVATE ${HOMEWORK_DIR})

add_custom_target(shaders
        ALL
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_vs.sc ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_fs.sc
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light_vs.sc ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light_fs.sc
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_vs.sc ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_fs.sc
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_vs.sc ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_fs.sc
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/uniforms.sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc
        )

add_custom_command(TARGET shaders
        PRE_BUILD
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/mesh_vs.bin --type v --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/mesh_fs.bin --type f --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/light_vs.bin --type v --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/light_fs.bin --type f --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/sky_vs.bin --type v --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/sky_fs.bin --type f --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/shadow_vs.bin --type v --platform windows
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/glsl/shadow_fs.bin --type f --platform windows

        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/mesh_vs.bin --type v --platform windows -p vs_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/mesh_fs.bin --type f --platform windows -p ps_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/light_vs.bin --type v --platform windows -p vs_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/light_fs.bin --type f --platform windows -p ps_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/sky_vs.bin --type v --platform windows -p vs_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/sky_fs.bin --type f --platform windows -p ps_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_vs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/shadow_vs.bin --type v --platform windows -p vs_5_0
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/shaderc.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/examples/common -i ${CMAKE_CURRENT_SOURCE_DIR}/bgfx/src/ -f ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_fs.sc -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders/dx11/shadow_fs.bin --type f --platform windows -p ps_5_0
        )

add_dependencies(homework shaders)

# Special Visual Studio Flags
if (MSVC)
    target_compile_definitions(homework PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif ()
