#pragma once
#include <cstdint>

#undef max
namespace IMAF 
{
	class Panel 
	{
	public:
		Panel();
		virtual ~Panel();
		const uint64_t GetId() const { return id; }

		virtual void UiRender() {};

	protected:
		uint64_t id;
	};
}