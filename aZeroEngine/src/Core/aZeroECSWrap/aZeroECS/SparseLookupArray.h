#pragma once
#include <vector>
#include <unordered_map>

namespace aZero
{
	namespace DataStructures
	{
		template <typename ElementType>
		class SparseLookupArray
		{
		private:
			std::vector<unsigned int> m_IDtoElementIndexMap;
			std::unordered_map<unsigned int, unsigned int> m_ElementIndexToIDMap;
			std::vector<ElementType> m_Elements;

		public:
			SparseLookupArray() = default;

			SparseLookupArray(unsigned int StartNumElements)
			{
				this->Init(StartNumElements);
			}

			void Init(unsigned int StartNumElements = 0)
			{
				m_IDtoElementIndexMap.resize(StartNumElements, UINT_MAX);
			}

			void ForceExpand(unsigned int AdditionalElements)
			{
				m_IDtoElementIndexMap.resize(m_IDtoElementIndexMap.size() + AdditionalElements, UINT_MAX);
			}

			void Add(unsigned int ID, ElementType&& Data)
			{
				if (ID >= m_IDtoElementIndexMap.size())
				{
					// Out-of-range
					throw;
				}

				if (m_IDtoElementIndexMap.at(ID) != UINT_MAX)
				{
					// Replace existing data
					m_Elements.at(m_IDtoElementIndexMap.at(ID)) = Data;
					return;
				}

				m_IDtoElementIndexMap.at(ID) = m_Elements.size(); // Replace this with the integer holding the index to avoid resizing the vec????
				m_ElementIndexToIDMap.emplace(m_Elements.size(), ID);
				m_Elements.emplace_back(std::forward<ElementType>(Data));
			}

			void Remove(unsigned int ID)
			{
				if (ID >= m_IDtoElementIndexMap.size())
				{
					if (ID == UINT_MAX)
					{
						return;
					}

					// Out-of-range
					throw;
				}
				const unsigned int ElementIndex = m_IDtoElementIndexMap.at(ID);

				if (ElementIndex == UINT_MAX)
				{
					return;
				}

				m_ElementIndexToIDMap.erase(ElementIndex);
				m_IDtoElementIndexMap.at(ID) = UINT_MAX;

				// Move last element to ElementIndex
				if (m_Elements.size() - 1 != 0)
				{
					m_Elements.at(ElementIndex) = m_Elements.at(m_Elements.size() - 1);
					const unsigned int LastElementID = m_ElementIndexToIDMap.at(m_Elements.size() - 1);
					m_IDtoElementIndexMap.at(LastElementID) = ElementIndex;
				}
				m_Elements.resize(m_Elements.size() - 1); // Change to holding the size with an int to avoid reallocs...
			}

			bool Exists(unsigned int ID) const
			{
				return m_IDtoElementIndexMap.at(ID) != UINT_MAX;
			}

			ElementType& Get(unsigned int ID)
			{
				return m_Elements[m_IDtoElementIndexMap[ID]];
			}

			const ElementType& Get(unsigned int ID) const
			{
				return m_Elements[m_IDtoElementIndexMap[ID]];
			}

			std::vector<ElementType>& GetArray() { return m_Elements; }
			const std::vector<ElementType>& GetArray() const { return m_Elements; }
		};
	}
}