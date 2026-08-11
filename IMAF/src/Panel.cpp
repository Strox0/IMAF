#include "IMAF/Panel.h"
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include "IMAF/Random.h"
#include <limits>

static std::unordered_set<uint64_t> m_ids;
static std::mutex m_mutex;

IMAF::Panel::Panel() : id(0)
{
	IMAF::Utils::Random::Number<uint64_t> g(0, std::numeric_limits<uint64_t>::max());
	while (true)
	{
		id = g.Gen();

		std::lock_guard<std::mutex> m(m_mutex);
		if (!m_ids.contains(id))
		{
			m_ids.insert(id);
			break;
		}
	}
}

IMAF::Panel::~Panel()
{
	std::lock_guard<std::mutex> m(m_mutex);
	m_ids.erase(id);
}