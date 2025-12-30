cmake_minimum_required(VERSION 3.10)

project(ImGuizmo)

add_library(ImGui STATIC
        example/imgui.h
        example/imgui.cpp
)

add_library(ImGuizmo STATIC
        example/ImApp.h
        ImGuizmo.h
        ImGuizmo.cpp
        example/main.cpp
)

target_include_directories(ImGuizmo PUBLIC
        example
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ImGuizmo PUBLIC ImGui)
