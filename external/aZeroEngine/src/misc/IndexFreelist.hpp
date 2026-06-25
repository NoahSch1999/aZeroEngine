#pragma once
#include <stack>
#include <vector>

namespace aZero
{
	template<typename T>
	concept IntegralTypeConcept = std::is_integral_v<T>;

	namespace DS
	{
		// todo Replace with https://github.com/NoahSch1999/aZero-IndexAllocator/tree/main
		template<IntegralTypeConcept IndexType>
		class IndexFreelist
		{
		private:
			std::stack<IndexType, std::vector<IndexType>> m_FreeIndices;
			IndexType m_NextFreeIndex;

		public:
			constexpr static IndexType m_InvalidIndex = std::numeric_limits<IndexType>::max();

			IndexFreelist() :m_NextFreeIndex(0) {}

			IndexType New()
			{
				if (!m_FreeIndices.empty())
				{
					IndexType index = m_FreeIndices.top();
					m_FreeIndices.pop();
					return index;
				}

				return m_NextFreeIndex++;
			}

			IndexType NewNext()
			{
				return m_NextFreeIndex++;
			}

			void Recycle(IndexType index)
			{
				m_FreeIndices.push(index);
			}

			IndexType GetMax() const { return m_NextFreeIndex; }
		};
	}
}
