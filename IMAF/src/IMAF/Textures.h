#pragma once
#include <unordered_map>
#include <mutex>
#include <vector>
#include <Windows.h>
#include <unordered_set>

namespace IMAF
{
	namespace Utils
	{
		namespace Textures
		{
			void QueueDelete(HGLRC ctx, unsigned int tid);
			void PreformDelete();
			void RegisterContext(HGLRC ctx);
			void RegisterSharedContext(HGLRC owner_ctx, HGLRC ctx);
			void RetireContext(HGLRC ctx);
			HGLRC GetOwnerContext(HGLRC ctx);
			void RegisterImage(void* image);
			void RetireImage(void* image);
		}
	}
}