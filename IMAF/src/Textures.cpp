#include "IMAF/Textures.h"
#include <Windows.h>
#include <mutex>
#include <vector>
#include <utility>
#include "gl/gl.h"
#include <unordered_map>
#include <unordered_set>

static std::mutex s_texture_mutex;
static std::unordered_map<HGLRC, std::vector<unsigned int>> s_ctx_textures;

void IMAF::Utils::Textures::QueueDelete(HGLRC ctx, unsigned int tid)
{
	if (ctx == nullptr || tid == 0)
		return;

	std::lock_guard tm(s_texture_mutex);
	auto it = s_ctx_textures.find(ctx);
	if (it == s_ctx_textures.end())
		return;
	it->second.push_back(tid);
}

void IMAF::Utils::Textures::PreformDelete()
{
	HGLRC curr_context = wglGetCurrentContext();
	if (curr_context == nullptr)
		return;

	std::vector<unsigned int> textures;
	{
		std::lock_guard m(s_texture_mutex);

		auto it = s_ctx_textures.find(curr_context);
		if (it == s_ctx_textures.end())
			return;

		textures.swap(it->second);
	}

	if (!textures.empty())
		glDeleteTextures(textures.size(), textures.data());
}

void IMAF::Utils::Textures::RegisterContext(HGLRC ctx)
{
	if (ctx == nullptr)
		return;

	std::lock_guard m(s_texture_mutex);
	s_ctx_textures.try_emplace(ctx);
}

void IMAF::Utils::Textures::RetireContext(HGLRC ctx)
{
	if (ctx == nullptr)
		return;

	std::lock_guard m(s_texture_mutex);
	s_ctx_textures.erase(ctx);
}
