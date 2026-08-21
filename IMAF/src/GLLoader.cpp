#include "IMAF/GLLoader.h"
#include <Windows.h>

IMAF::Utils::FunctionDefs::glBindBuffer IMAF::Utils::LoadGlBindBuffer()
{
	PROC address = wglGetProcAddress("glBindBuffer");

	if (address == nullptr ||
		address == reinterpret_cast<PROC>(1) ||
		address == reinterpret_cast<PROC>(2) ||
		address == reinterpret_cast<PROC>(3) ||
		address == reinterpret_cast<PROC>(-1))
	{
		return nullptr;
	}

	return reinterpret_cast<IMAF::Utils::FunctionDefs::glBindBuffer>(address);
}
