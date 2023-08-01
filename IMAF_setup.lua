outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

workspace "IMAF"
	architecture "x64"
	configurations {"Debug","Release"}

	filter "system:windows"
    buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

include "IMAF"
include "Test_App"