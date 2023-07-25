outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

workspace "IMAF"
	architecture "x64"
	configurations {"Debug","Release"}

	filter "system:windows"
    buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

include "vendor/glfw"

project "IMAF"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	location "IMAF"

	files {
		"IMAF/src/**.cpp",
		"IMAF/src/**.h",
		"vendor/imgui/**.cpp",
		"vendor/imgui/**.h",
		"vendor/opengl/api/**.h"
	}

	includedirs {
		"IMAF/src",
		"vendor/imgui",
		"vendor/glfw/include",
		"vendor/opengl/api"
	}

	links {
		"glfw"		
	}

	targetdir ("IMAF/bin/" .. outputdir .. "/")
	objdir ("IMAF/intermediate/" .. outputdir .. "/")

	filter "system:windows"
      systemversion "latest"

	filter "configurations:Debug"
      defines { "_DEBUG" }
      runtime "Debug"
      symbols "On"

	filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
      symbols "Off"

project "Test_App"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	location "Test_App/"

	targetdir ("IMAF/bin/" .. outputdir .. "/")
	objdir ("IMAF/intermediate/" .. outputdir .. "/")

	files {
		"Test_App/src/**.cpp",
		"Test_App/src/**.h"
	}

	includedirs {
		"Test_App/src/",
		"vendor/imgui",
		"vendor/glfw",
		"vendor/opengl"
	}

	links {
		"IMAF"
	}

	filter "system:windows"
      systemversion "latest"

	filter "configurations:Debug"
      defines { "_DEBUG" }
      runtime "Debug"
      symbols "On"

	filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
      symbols "Off"