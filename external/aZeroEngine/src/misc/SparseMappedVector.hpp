#pragma once
#include <unordered_map>
#include <span>

namespace aZero
{
	namespace DataStructures
	{
		template<typename IDType, typename Type>
		class SparseMappedVector
		{
		private:
			std::unordered_map<IDType, uint32_t> m_ID_To_Index;
			std::unordered_map<uint32_t, IDType> m_Index_To_ID;
			std::vector<Type> m_Data;
			uint32_t m_CurrentLastIndex = 0;

		public:
			SparseMappedVector() = default;

			void AddOrUpdate(IDType ID, Type&& Primitive)
			{
				if (auto Index = m_ID_To_Index.find(ID); Index != m_ID_To_Index.end())
				{
					m_Data.at(Index->second) = Primitive;
				}
				else
				{
					m_ID_To_Index[ID] = m_CurrentLastIndex;
					m_Index_To_ID[m_CurrentLastIndex] = ID;

					if (m_Data.size() > m_CurrentLastIndex)
					{
						m_Data.at(m_CurrentLastIndex) = Primitive;
					}
					else
					{
						m_Data.emplace_back(Primitive);
					}
					m_CurrentLastIndex++;
				}
			}

			void AddOrUpdate(IDType ID, const Type& Primitive)
			{
				if (auto Index = m_ID_To_Index.find(ID); Index != m_ID_To_Index.end())
				{
					m_Data.at(Index->second) = Primitive;
				}
				else
				{
					m_ID_To_Index[ID] = m_CurrentLastIndex;
					m_Index_To_ID[m_CurrentLastIndex] = ID;

					if (m_Data.size() > m_CurrentLastIndex)
					{
						m_Data.at(m_CurrentLastIndex) = Primitive;
					}
					else
					{
						m_Data.emplace_back(Primitive);
					}
					m_CurrentLastIndex++;
				}
			}

			void Remove(IDType ID)
			{
				if (auto Index = m_ID_To_Index.find(ID); Index != m_ID_To_Index.end())
				{
					if (Index->second != m_CurrentLastIndex)
					{
						// Copy last element index data to removed index data
						const uint32_t LastElementIndex = m_CurrentLastIndex - 1;
						m_Data[Index->second] = std::move(m_Data.at(LastElementIndex));

						m_Data.at(LastElementIndex) = Type(); // Reset

						// Remap entity=>index map so moved entityid points to removed index
						const IDType LastID = m_Index_To_ID.at(LastElementIndex);
						m_ID_To_Index.at(LastID) = Index->second;

						// Remap so that index=>entity map so removed index points to last entity
						m_Index_To_ID.at(Index->second) = LastID;

						// Remove old entity id from entity=>index map
						m_ID_To_Index.erase(ID);

						// Remove last index from index=>entity
						m_Index_To_ID.erase(LastElementIndex);
					}
					else
					{
						m_Data.at(Index->second) = Type(); // Reset
					}

					m_CurrentLastIndex--;
				}
			}

			void ShrinkToFit()
			{
				if (m_CurrentLastIndex)
				{
					m_Data.resize(m_CurrentLastIndex);
				}
			}

			const std::vector<Type>& GetData() const { return m_Data; }

			bool Contains(const IDType& ID) const { return m_ID_To_Index.contains(ID); }
		};
	}
}