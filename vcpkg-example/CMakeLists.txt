cmake_minimum_required(VERSION 3.16)

project(vcpkg-example)

add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
find_package(Stb REQUIRED)

find_package(imgui     CONFIG REQUIRED)
find_package(imguizmo  CONFIG REQUIRED)

add_executable(example-app)

target_sources(example-app PRIVATE main.cpp)

target_compile_options(example-app PRIVATE "/std:c++17")

target_include_directories(example-app PRIVATE
    ${Stb_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../example
)

target_link_libraries(example-app PRIVATE
    imgui::imgui
    imguizmo::imguizmo
)
