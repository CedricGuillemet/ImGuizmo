cmake_minimum_required(VERSION 3.25)
project(ImGuizmo)

set(SOURCES
   "GraphEditor.cpp"
   "ImCurveEdit.cpp"
   "ImGradient.cpp"
   "ImGuizmo.cpp"
   "ImSequencer.cpp"
)

set(HEADERS
   "GraphEditor.h"
   "ImCurveEdit.h"
   "ImGradient.h"
   "ImGuizmo.h"
   "ImSequencer.h"
   "ImZoomSlider.h"
)

add_library(ImGuizmo STATIC
   ${SOURCES}
   ${HEADERS}
)

target_include_directories(ImGuizmo PUBLIC
   "."
)

target_link_libraries(ImGuizmo PRIVATE
   ImGui
)

