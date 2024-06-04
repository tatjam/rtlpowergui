cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(hitl)

set(HELLOIMGUI_USE_FREETYPE OFF CACHE BOOL "Use freetype for imoji support in HelloImGui" FORCE)
add_subdirectory(hello_imgui)

include_directories(implot)
file(GLOB_RECURSE SOURCES src/*.cpp)

if (PROJECT_IS_TOP_LEVEL AND UNIX)
    # Create symlink to compile_commands.json for IDE to pick it up
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    )
endif()

hello_imgui_add_app(rtlpowergui ${SOURCES} implot/implot.cpp implot/implot_items.cpp)

