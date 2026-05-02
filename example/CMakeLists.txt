cmake_minimum_required(VERSION 3.16)
project(ImGuizmoExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_include_directories(imgui PUBLIC ${stb_SOURCE_DIR})

set(IMGUIZMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(imguizmo STATIC
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
    ${IMGUIZMO_DIR}/GraphEditor.cpp
    ${IMGUIZMO_DIR}/ImCurveEdit.cpp
    ${IMGUIZMO_DIR}/ImGradient.cpp
    ${IMGUIZMO_DIR}/ImSequencer.cpp
    ${IMGUIZMO_DIR}/ImVectorEditor.cpp
)
target_include_directories(imguizmo PUBLIC ${IMGUIZMO_DIR})
target_link_libraries(imguizmo PUBLIC imgui)

add_executable(example-app main.cpp)
target_link_libraries(example-app PRIVATE imguizmo)
target_include_directories(example-app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(example-app PRIVATE opengl32 imm32 gdi32)
elseif(APPLE)
    target_compile_definitions(example-app PRIVATE GL_SILENCE_DEPRECATION)
    target_compile_definitions(imgui PRIVATE GL_SILENCE_DEPRECATION)
    target_link_libraries(example-app PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework Carbon"
        "-framework IOKit"
    )
elseif(UNIX)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(example-app PRIVATE X11::X11 OpenGL::GL ${CMAKE_DL_LIBS})
endif()
