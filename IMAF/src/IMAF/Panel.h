#pragma once
#include <cstdint>

#undef max
namespace IMAF 
{
	class Panel 
	{
	public:
		Panel();
		Panel(Panel&&) = delete;
		Panel(const Panel&) = delete;
		Panel& operator=(const Panel&) = delete;
		Panel& operator=(Panel&&) = delete;
		virtual ~Panel();
		const uint64_t GetId() const { return id; }

		virtual void UiRender() {};

	private:
		const uint64_t id;
	};
}