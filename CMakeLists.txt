cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)

set(project imguizmo)
set(version 1.79)

project(${project})

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
add_library(${PROJECT_NAME} libmain.cpp)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

find_package(imgui REQUIRED COMPONENTS imgui-core)

option(IMGUIZMO_GUIZMO "Build ImGuizmo" ON)
if (${IMGUIZMO_GUIZMO})
    add_subdirectory(imguizmo-guizmo)
    target_link_libraries(imguizmo imguizmo::imguizmo-guizmo)
endif()

option(IMGUIZMO_SEQUENCER "Build ImSequencer" ON)
if (${IMGUIZMO_SEQUENCER})
    add_subdirectory(imguizmo-sequencer)
    target_link_libraries(imguizmo imguizmo::imguizmo-sequencer)
endif()

option(IMGUIZMO_GRAPHEDITOR "Build GraphEditor" ON)
if (${IMGUIZMO_GRAPHEDITOR})
    add_subdirectory(imguizmo-grapheditor)
    target_link_libraries(imguizmo imguizmo::imguizmo-grapheditor)
endif()

option(IMGUIZMO_CURVEEDIT "Build CurveEdit" ON)
if (${IMGUIZMO_CURVEEDIT})
    add_subdirectory(imguizmo-curveedit)
    target_link_libraries(imguizmo imguizmo::imguizmo-curveedit)
endif()

option(IMGUIZMO_GRADIENT "Build CurveEdit" ON)
if (${IMGUIZMO_GRADIENT})
    add_subdirectory(imguizmo-gradient)
    target_link_libraries(imguizmo imguizmo::imguizmo-gradient)
endif()

option(IMGUIZMO_ZOOMSLIDER "Build ZoomSlider" ON)
if (${IMGUIZMO_ZOOMSLIDER})
    add_subdirectory(imguizmo-zoomslider)
    target_link_libraries(imguizmo imguizmo::imguizmo-zoomslider)
endif()

option(IMGUIZMO_BUILD_EXAMPLE "Build example" ON)
if (${IMGUIZMO_BUILD_EXAMPLE})
    add_subdirectory(example)
endif()
