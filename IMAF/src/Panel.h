#pragma once
#include "Random.h"
#include <limits>

namespace IMAF 
{
	class Panel 
	{
	public:
		Panel() : id(IMAF::Utils::Random::Number<uint64_t>(0, std::numeric_limits<uint64_t>().max()).Gen()) {}
		const uint64_t GetId() const { return id; }

		virtual void UiRender() {};

	protected:
		const uint64_t id;
	};
}