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
		"vendor/opengl/api",
		"vendor/KHR"
	}

	links {
		"glfw",
		"opengl32"
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