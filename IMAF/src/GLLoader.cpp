#include "IMAF/GLLoader.h"
#include <Windows.h>

bool IMAF::Utils::LoadGlFunctions()
{
	PROC address = wglGetProcAddress("glBindBuffer");

	if (address == nullptr ||
		address == reinterpret_cast<PROC>(1) ||
		address == reinterpret_cast<PROC>(2) ||
		address == reinterpret_cast<PROC>(3) ||
		address == reinterpret_cast<PROC>(-1))
	{
		return true;
	}

	IMAF::Utils::glBindBuffer = reinterpret_cast<IMAF::Utils::FunctionDefs::glBindBuffer>(address);

	return false;
}
