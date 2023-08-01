#pragma once
#include <concepts>
#include <random>

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

				type Gen()
				{
					static std::random_device dev;
					static std::mt19937 rng(dev());

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
				type m_min;
				type m_max;
			};
		}
	}
}