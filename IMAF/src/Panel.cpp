#include "IMAF/Panel.h"
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include "IMAF/Random.h"
#include <limits>

namespace
{
	struct PanelRegistry
	{
		std::mutex mutex;
		std::unordered_set<uint64_t> ids;
	};

	PanelRegistry& GetPanelRegistry()
	{
		static PanelRegistry* const pr = new PanelRegistry();
		return *pr;
	}

	uint64_t GenId()
	{
		IMAF::Utils::Random::Number<uint64_t> g(0, std::numeric_limits<uint64_t>::max());

		PanelRegistry& pr = GetPanelRegistry();

		uint64_t id = 0;
		while (true)
		{
			id = g.Gen();

			std::lock_guard<std::mutex> m(pr.mutex);
			if (!pr.ids.contains(id))
			{
				pr.ids.insert(id);
				break;
			}
		}
		return id;
	}
}

IMAF::Panel::Panel() : id(GenId())
{

}

IMAF::Panel::~Panel()
{
	PanelRegistry& pr = GetPanelRegistry();

	std::lock_guard<std::mutex> m(pr.mutex);
	pr.ids.erase(id);
}