project "Test_App"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/")
	objdir ("intermediate/" .. outputdir .. "/")

	files {
		"src/**.cpp",
		"src/**.h"
	}

	includedirs {
		"src/",
		"../vendor/imgui",
		"../vendor/glfw/include",
		"../vendor/opengl/api",
		"../IMAF/src"
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