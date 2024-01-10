cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

project(imguizmo DESCRIPTION "Immediate mode 3D gizmo for scene editing and other controls based on Dear Imgui")

find_package(imgui QUIET COMPONENTS imgui-core)

option(BUILD_SHARED_LIBS "Build shared libs" OFF)

# Create the main target
add_library(${PROJECT_NAME} 
    GraphEditor.cpp
    ImCurveEdit.cpp
    ImGradient.cpp
    ImGuizmo.cpp
    ImSequencer.cpp)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:ImGuizmo.h>
    $<INSTALL_INTERFACE:GraphEditor.h>
    $<INSTALL_INTERFACE:ImCurveEdit.h>
    $<INSTALL_INTERFACE:ImGradient.h>
    $<INSTALL_INTERFACE:ImGuizmo.h>
    $<INSTALL_INTERFACE:ImSequencer.h>
    $<INSTALL_INTERFACE:ImZoomSlider.h>)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        imgui-core)

#option(IMGUIZMO_BUILD_EXAMPLE "Build example" ON)
#if (${IMGUIZMO_BUILD_EXAMPLE})
#    add_subdirectory(example)
#endif()    

# Export and install targets and headers
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(EXPORT ${PROJECT_NAME}-targets
    FILE
        ${PROJECT_NAME}Targets.cmake
    NAMESPACE
        ${PROJECT_NAME}::
    DESTINATION
        ${INSTALL_CONFIGDIR})

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
 )

install(FILES 
        GraphEditor.h
        ImCurveEdit.h
        ImGradient.h
        ImGuizmo.h
        ImSequencer.h
        ImZoomSlider.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

# Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

export(EXPORT ${PROJECT_NAME}-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::)

# Register package in user's package registry
export(PACKAGE ${PROJECT_NAME})