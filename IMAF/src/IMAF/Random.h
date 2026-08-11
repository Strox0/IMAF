#pragma once
#include <random>
#include <type_traits>

namespace IMAF {
	namespace Utils {
		namespace Random {
			template<typename _T>
			concept Integral = std::is_integral_v<_T>;

			// concept for floating point types
			template<typename _T>
			concept Real = std::is_floating_point_v<_T>;

			template<typename T>
			class Number {
			public:
				typedef T type;
				Number(type min, type max) :m_min(min), m_max(max) {};

				void SetRange(type min, type max)
				{
					m_min = min;
					m_max = max;
				}

				constexpr type Gen() const
				{
					thread_local std::mt19937_64 rng(GenSeed());

					if constexpr (Integral<T>) {
						std::uniform_int_distribution<type> dist(m_min, m_max);
						return dist(rng);
					}
					else if constexpr (Real<T>) {
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