cmake_minimum_required(VERSION 3.16)
project(ImGuizmo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imguizmo STATIC
    ImGuizmo.cpp
    GraphEditor.cpp
    ImCurveEdit.cpp
    ImGradient.cpp
    ImSequencer.cpp
    ImVectorEditor.cpp
)

add_library(imguizmo::imguizmo ALIAS imguizmo)

target_include_directories(imguizmo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

find_package(imgui CONFIG QUIET)
if(imgui_FOUND)
    target_link_libraries(imguizmo PUBLIC imgui::imgui)
elseif(TARGET imgui)
    target_link_libraries(imguizmo PUBLIC imgui)
else()
    message(FATAL_ERROR
        "ImGuizmo requires Dear ImGui. Provide it via find_package(imgui) "
        "or define an 'imgui' target before add_subdirectory(ImGuizmo)."
    )
endif()
