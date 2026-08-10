#pragma once
#include <cstdint>

#undef max
namespace IMAF 
{
	// ISSUE 9: Panel is polymorphic but has no virtual destructor, so deletion through a base-owned pointer can be undefined.
	//FIXED
	class Panel 
	{
	public:
		Panel();
		virtual ~Panel() = default;
		const uint64_t GetId() const { return id; }

		virtual void UiRender() {};

	protected:
		uint64_t id;
	};
}