cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)

project(hitl)

set(HELLOIMGUI_USE_FREETYPE OFF CACHE BOOL "Use freetype for imoji support in HelloImGui" FORCE)
add_subdirectory(hello_imgui)

file(GLOB_RECURSE SOURCES src/*.cpp)

hello_imgui_add_app(rtlpowergui ${SOURCES} implot/implot.cpp implot/implot_items.cpp)

