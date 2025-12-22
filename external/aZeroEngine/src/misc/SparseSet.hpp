#pragma once
#include <vector>
#include <optional>

namespace aZero
{
	namespace DS
	{
		/// <summary>
		/// Concept which checks whether or not type T is an integral.
		/// </summary>
		template<typename T>
		concept IntegralType = std::is_integral_v<T>;

		/// <summary>
		/// A sparse set. 
		/// Maps IDs to elements in a dense array.
		/// </summary>
		/// <typeparam name="ElementType">Type of the elements</typeparam>
		/// <typeparam name="IDType">Type of the IDs</typeparam>
		template<IntegralType IDType, typename ElementType>
		class SparseSet
		{
		private:

			/// <summary>
			/// The last value of IDType is used for checking if the index is valid or not.
			/// This reduces the cost of "exist" checks by moving it into the resize and removal sections.
			/// </summary>
			static constexpr IDType m_EmptyIndex{ std::numeric_limits<IDType>::max() };

			/// <summary>
			/// Maps IDs to the element indices in the dense array.
			/// </summary>
			std::vector<IDType> m_ID_To_Element;

			/// <summary>
			/// Maps the element indices in the dense array to the IDs.
			/// </summary>
			std::vector<IDType> m_Element_To_ID;

			/// <summary>
			/// Contains the elements.
			/// </summary>
			std::vector<ElementType> m_Elements;

			/// <summary>
			/// The current last element that is referenced by an ID.
			/// </summary>
			IDType m_CurrentLast = 0;

		public:

			/// <summary>
			/// Default initialize with zero. Requires the SparseSet to be manually resized via an available function.
			/// </summary>
			SparseSet() = default;

			/// <summary>
			/// Initializes with the specified number of supported elements.
			/// </summary>
			/// <param name="NumElements">Num elements to support</param>
			SparseSet(IDType NumElements)
			{
				this->Init(NumElements);
			}

			void Init(IDType NumElements)
			{
				m_ID_To_Element.resize(NumElements, m_EmptyIndex);
				m_Element_To_ID.resize(0);
				m_Elements.resize(0);
				m_CurrentLast = 0;
			}

			/// <summary>
			/// Adds an entry for the ID if it doesn't already exist.
			/// Copies the element.
			/// </summary>
			/// <param name="ID">ID to add for</param>
			/// <param name="Element">Element to add</param>
			void Add(IDType ID, const ElementType& Element)
			{
				if (!this->Exists(ID))
				{
					if (m_CurrentLast < m_Elements.size())
					{
						m_Elements.at(m_CurrentLast) = Element;
						m_Element_To_ID.at(m_CurrentLast) = ID;
					}
					else
					{
						m_Elements.push_back(Element);
						m_Element_To_ID.push_back(ID);
					}
					m_ID_To_Element.at(ID) = m_CurrentLast;
					m_CurrentLast++;
				}
			}

			/// <summary>
			/// Adds an entry for the ID if it doesn't already exist.
			/// Calls the move operator for the element.
			/// </summary>
			/// <param name="ID">ID to add an entry for</param>
			/// <param name="Element">Element to add</param>
			void Add(IDType ID, ElementType&& Element)
			{
				if (!this->Exists(ID))
				{
					if (m_CurrentLast < m_Elements.size())
					{
						m_Elements.at(m_CurrentLast) = std::move(Element);
						m_Element_To_ID.at(m_CurrentLast) = ID;
					}
					else
					{
						m_Elements.push_back(std::move(Element));
						m_Element_To_ID.push_back(ID);
					}
					m_ID_To_Element.at(ID) = m_CurrentLast;
					m_CurrentLast++;
				}
			}

			/// <summary>
			/// Removes the entry for the input ID if it exists.
			/// </summary>
			/// <param name="ID">ID of the entry to remove</param>
			void Remove(IDType ID)
			{
				if (this->Exists(ID))
				{
					const IDType LastIndex = m_CurrentLast - 1;
					const IDType RemovedElementIndex = m_ID_To_Element.at(ID);
					if (RemovedElementIndex != LastIndex)
					{
						m_Elements.at(RemovedElementIndex) = std::move(m_Elements.at(LastIndex));
						const IDType LastElementID = m_Element_To_ID.at(LastIndex);
						m_ID_To_Element.at(LastElementID) = RemovedElementIndex;
						m_Element_To_ID.at(RemovedElementIndex) = LastElementID;
					}
					m_ID_To_Element.at(ID) = m_EmptyIndex;
					m_CurrentLast--;
				}
			}

			/// <summary>
			/// Fetches a reference to the entry.
			/// Throws an exception if the ID doesn't have an entry.
			/// NOTE: Reference might become invalid after certain operations on the SparseSet.
			/// </summary>
			/// <param name="ID">ID to get the entry for</param>
			/// <returns></returns>
			ElementType& Get(IDType ID)
			{
				const IDType elementIndex = m_ID_To_Element.at(ID);
				return m_Elements.at(elementIndex);
			}

			/// <summary>
			/// Fetches a const reference to the entry.
			/// Throws an exception if the ID doesn't have an entry.
			/// NOTE: Reference might become invalid after certain operations on the SparseSet.
			/// </summary>
			/// <param name="ID">ID to get the entry for</param>
			/// <returns></returns>
			const ElementType& Get(IDType ID) const
			{
				const IDType elementIndex = m_ID_To_Element.at(ID);
				return m_Elements.at(elementIndex);
			}

			/// <summary>
			/// Fetches an optional reference to the entry.
			/// If the ID isn't valid, the optional is empty.
			/// NOTE: Reference might become invalid after certain operations on the SparseSet.
			/// </summary>
			/// <param name="ID">ID to get the entry for</param>
			/// <returns></returns>
			std::optional<std::reference_wrapper<ElementType>> GetIfExists(IDType ID)
			{
				if (this->Exists(ID))
				{
					return std::optional<std::reference_wrapper<ElementType>>{std::ref(m_Elements.at(m_ID_To_Element.at(ID)))};
				}
				else
				{
					return std::nullopt;
				}
			}

			/// <summary>
			/// Fetches an optional const reference to the entry.
			/// If the ID isn't valid, the optional is empty.
			/// NOTE: Reference might become invalid after certain operations on the SparseSet.
			/// </summary>
			/// <param name="ID">ID to get the entry for</param>
			/// <returns></returns>
			std::optional<std::reference_wrapper<const ElementType>> GetIfExists(IDType ID) const
			{
				if (this->Exists(ID))
				{
					return std::optional<std::reference_wrapper<const ElementType>>{std::ref(m_Elements.at(m_ID_To_Element.at(ID)))};
				}
				else
				{
					return std::nullopt;
				}
			}

			/// <summary>
			/// Returns a reference to the contiguous array of elements.
			/// </summary>
			/// <returns></returns>
			std::vector<ElementType>& GetData() { return m_Elements; }

			/// <summary>
			/// Returns a constant reference to the contiguous array of elements.
			/// </summary>
			/// <returns></returns>
			const std::vector<ElementType>& GetData() const { return m_Elements; }

			/// <summary>
			/// Checks whether an entry for the input ID exists.
			/// </summary>
			/// <param name="ID">ID check an entry for</param>
			/// <returns></returns>
			bool Exists(IDType ID) const
			{
				if (ID >= m_ID_To_Element.size())
					return false;

				const IDType elementIndex = m_ID_To_Element.at(ID);
				return elementIndex != m_EmptyIndex;
			}

			/// <summary>
			/// Shrinks the internal dense arrays so the SparseSet only have the least amount of memory allocated for the currently stored elements.
			/// </summary>
			void ShrinkToFit()
			{
				m_Elements.resize(m_CurrentLast);
				m_Elements.shrink_to_fit();
				m_Element_To_ID.resize(m_CurrentLast);
				m_Element_To_ID.shrink_to_fit();
			}

			/// <summary>
			/// Resizes the SparseSet so it supports the input number of entries.
			/// Nothing is done if the input number of entries is less or equal than the current max.
			/// </summary>
			/// <param name="NumElements">Number of entries to support</param>
			void ExtendTo(IDType NumEntries)
			{
				if (NumEntries > m_ID_To_Element.size())
				{
					m_ID_To_Element.resize(NumEntries, m_EmptyIndex);
				}
			}

			/// <summary>
			/// Returns how many entries the SparseSet supports.
			/// </summary>
			/// <returns></returns>
			IDType NumSupportedElements() const
			{
				return m_ID_To_Element.size();
			}

			/// <summary>
			/// Returns the number of elements that the dense array has allocated space for.
			/// Calls std::vector<ElementType>::capacity().
			/// </summary>
			/// <returns></returns>
			IDType InternalSize() const
			{
				return m_Elements.capacity();
			}

			IDType GetElementIndex(IDType ID) const { return m_ID_To_Element.at(ID); }
		};
	}
}