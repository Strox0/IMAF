#pragma once
#include <random>
#include <type_traits>
#include <stdexcept>
#include <limits>

namespace IMAF {
	namespace Utils {
		namespace Random {
			template<typename _T>
			concept Integral = std::is_integral_v<_T>;

			// concept for floating point types
			template<typename _T>
			concept Real = std::is_floating_point_v<_T>;

			template<typename _T>
			concept NumberType =
				std::is_same_v<_T, short> || std::is_same_v<_T, unsigned short> ||
				std::is_same_v<_T, int> || std::is_same_v<_T, unsigned int> ||
				std::is_same_v<_T, long> || std::is_same_v<_T, unsigned long> ||
				std::is_same_v<_T, long long> || std::is_same_v<_T, unsigned long long> ||
				std::is_same_v<_T, float> || std::is_same_v<_T, double> ||
				std::is_same_v<_T, long double>;

			template<NumberType type>
			class Number {
			public:
				Number(type min, type max) 
				{
					SetRange(min, max);
				};

				void SetRange(type min, type max)
				{
					if (max < min)
						throw std::invalid_argument("Max is smaller then min");

					if constexpr (Real<type>)
					{
						if (!std::isfinite(min) || !std::isfinite(max))
							throw std::invalid_argument("Random number range must be finite");

						if (max - min > std::numeric_limits<type>::max())
							throw std::invalid_argument("Random number range is too large");
					}

					m_min = min;
					m_max = max;
				}

				constexpr type Gen() const
				{
					thread_local std::mt19937_64 rng(GenSeed());

					if constexpr (Integral<type>) {
						std::uniform_int_distribution<type> dist(m_min, m_max);
						return dist(rng);
					}
					else if constexpr (Real<type>) {
						std::uniform_real_distribution<type> dist(m_min, m_max);
						return dist(rng);
					}
				}

			private:

				static std::mt19937_64::result_type GenSeed()
				{
					std::random_device rd;
					return rd();
				}

				type m_min;
				type m_max;
			};
		}
	}
}