include(FetchContent)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)
target_include_directories(imgui PUBLIC ${stb_SOURCE_DIR})

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
