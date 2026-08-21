#pragma once

namespace IMAF
{
	namespace Utils
	{
		namespace FunctionDefs
		{
			typedef void(__stdcall* glBindBuffer)(unsigned int target, unsigned int buffer);
		}

		FunctionDefs::glBindBuffer LoadGlBindBuffer();
	}
}