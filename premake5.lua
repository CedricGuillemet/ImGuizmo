project "ImGuizmo"
	kind "StaticLib"
	language "C++"
        cppdialect "C++20"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"GraphEditor.h",
		"GraphEditor.cpp",
		"ImCurveEdit.h",
		"ImCurveEdit.cpp",
		"ImGradient.h",
		"ImGradient.cpp",
		"ImGuizmo.h",
		"ImGuizmo.cpp",
		"ImSequencer.h",
		"ImSequencer.cpp",
		"ImZoomSlider.h",
	}

	includedirs
	{
		"%{IncludeDir.ImGui}",
	}

	links 
	{
		"ImGui",
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"