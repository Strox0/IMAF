#include "IMAF/Textures.h"
#include "IMAF/Image.h"
#include <Windows.h>
#include <mutex>
#include <vector>
#include <utility>
#include "gl/gl.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
	struct TextureRegistry
	{
		std::mutex texture_mutex;
		std::mutex image_mutex;
		std::unordered_map<HGLRC, std::vector<unsigned int>> ctx_textures;
		std::unordered_map<HGLRC, HGLRC> context_owners;
		std::vector<IMAF::Image*> registered_images;
	};

	TextureRegistry& GetTextureRegistry()
	{
		static TextureRegistry* const tr = new TextureRegistry();
		return *tr;
	}
}

void IMAF::Utils::Textures::QueueDelete(HGLRC ctx, unsigned int tid)
{
	if (ctx == nullptr || tid == 0)
		return;

	TextureRegistry& tr = GetTextureRegistry();

	std::lock_guard tm(tr.texture_mutex);
	auto owner = tr.context_owners.find(ctx);
	if (owner == tr.context_owners.end())
		return;

	auto it = tr.ctx_textures.find(owner->second);
	if (it == tr.ctx_textures.end())
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
		TextureRegistry& tr = GetTextureRegistry();
		std::lock_guard m(tr.texture_mutex);
		auto owner = tr.context_owners.find(curr_context);
		if (owner == tr.context_owners.end())
			return;

		auto it = tr.ctx_textures.find(owner->second);
		if (it == tr.ctx_textures.end())
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

	TextureRegistry& tr = GetTextureRegistry();
	std::lock_guard m(tr.texture_mutex);
	tr.ctx_textures.try_emplace(ctx);
	tr.context_owners.insert_or_assign(ctx, ctx);
}

void IMAF::Utils::Textures::RegisterSharedContext(HGLRC owner_ctx, HGLRC ctx)
{
	if (ctx == nullptr || owner_ctx == nullptr)
		return;

	TextureRegistry& tr = GetTextureRegistry();
	std::lock_guard m(tr.texture_mutex);
	auto owner = tr.context_owners.find(owner_ctx);
	if (owner == tr.context_owners.end())
		return;

	tr.context_owners.insert_or_assign(ctx, owner->second);
}

void IMAF::Utils::Textures::RetireContext(HGLRC ctx)
{
	if (ctx == nullptr)
		return;

	TextureRegistry& tr = GetTextureRegistry();
	std::vector<HGLRC> retired_contexts;

	{
		std::lock_guard m(tr.texture_mutex);
		auto owner = tr.context_owners.find(ctx);
		if (owner == tr.context_owners.end())
			return;

		HGLRC owner_ctx = owner->second;

		if (owner_ctx == ctx)
		{
			for (auto it = tr.context_owners.begin(); it != tr.context_owners.end();)
			{
				if (it->second == owner_ctx)
				{
					retired_contexts.push_back(it->first);
					it = tr.context_owners.erase(it);
				}
				else
					++it;
			}

			tr.ctx_textures.erase(owner_ctx);
		}
		else
			tr.context_owners.erase(owner);
	}

	std::lock_guard m(tr.image_mutex);
	for (const auto& c : retired_contexts)
	{
		for (auto& i : tr.registered_images)
		{
			i->DestroyRetiredContextReferences(c);
		}
	}
}

HGLRC IMAF::Utils::Textures::GetOwnerContext(HGLRC ctx)
{
	TextureRegistry& tr = GetTextureRegistry();
	std::lock_guard m(tr.texture_mutex);

	auto owner = tr.context_owners.find(ctx);
	if (owner == tr.context_owners.end())
		return nullptr;

	return owner->second;
}

void IMAF::Utils::Textures::RegisterImage(void* image)
{
	TextureRegistry& tr = GetTextureRegistry();
	std::lock_guard m(tr.image_mutex);
	tr.registered_images.push_back((IMAF::Image*)image);
}

void IMAF::Utils::Textures::RetireImage(void* image)
{
	TextureRegistry& tr = GetTextureRegistry();
	std::lock_guard m(tr.image_mutex);
	std::erase_if(tr.registered_images, [&image](IMAF::Image* img)
		{
			return image == (void*)img;
		}
	);
}