include "../vendor/glfw"

project "IMAF"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	files {
		"src/**.cpp",
		"src/**.h",
		"../vendor/imgui/**.cpp",
		"../vendor/imgui/**.h",
		"../vendor/opengl/api/**.h"
	}

	includedirs {
		"src",
		"../vendor/imgui",
		"../vendor/glfw/include",
		"../vendor/opengl/api",
		"../vendor/khr"
	}

	links {
		"glfw",
		"opengl32"
	}

	targetdir ("bin/" .. outputdir .. "/")
	objdir ("bin-int/" .. outputdir .. "/")

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