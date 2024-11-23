#pragma once
#include <set>

namespace aZero
{
	namespace DS
	{
		template<typename IndexType>
		class UniqueIndexList
		{
		private:
			std::set<IndexType> m_ReusedIndices;
			IndexType m_NextFreeIndex = (IndexType)0;

		public:
			UniqueIndexList() = default;

			const IndexType& GetIndex()
			{
				IndexType NextIndex;
				if (!m_ReusedIndices.empty())
				{
					NextIndex = *m_ReusedIndices.begin();
					m_ReusedIndices.erase(NextIndex);
				}

				NextIndex = m_NextFreeIndex;
				m_NextFreeIndex++;
				return NextIndex;
			}

			void RecycleIndex(const IndexType& ToRecycle)
			{
				if (ToRecycle < m_NextFreeIndex)
				{
					m_ReusedIndices.insert(ToRecycle);
				}
			}
		};
	}
}