#pragma once
#include "Entity.hpp"
#include "SparseLookupArray.hpp"

namespace aZero
{
	namespace ECS
	{
		/** @brief Stores components in a sparse array which can be accessed at constant time using an EntityID */
		template<typename ComponentType>
		class ComponentArray
		{
		private:
			aZero::DataStructures::SparseLookupArray<ComponentType> m_ComponentArray;
			int m_AdditionalElementsWhenResize = 1;

		public:
			ComponentArray() = default;

			ComponentArray(unsigned int StartNumElements, int AdditionalElementsWhenResize = 1)
			{
				this->Init(StartNumElements);
			}

			~ComponentArray() = default;

			void Init(unsigned int StartNumElements = 0, int AdditionalElementsWhenResize = 1)
			{
				m_AdditionalElementsWhenResize = AdditionalElementsWhenResize;
				m_ComponentArray.Init(StartNumElements);
			}

			/** Returns a pointer to a component tied to the input aZero::ECS::Entity
			* Returns nullptr if the aZero::ECS::Entity doesn't have a component of type ComponentType
			@param Ent
			@return ComponentType*
			*/
			ComponentType* GetComponent(const Entity& Ent)
			{
				if (!m_ComponentArray.Exists(Ent.GetID()))
				{
					return nullptr;
				}
				return m_ComponentArray.Get(Ent.GetID());
			}

			/** Returns a pointer to a const component tied to the input aZero::ECS::Entity
			* Returns nullptr if the aZero::ECS::Entity doesn't have a component of type ComponentType
			@param Ent
			@return const ComponentType*
			*/
			const ComponentType* GetComponent(const Entity& Ent) const
			{
				if (!m_ComponentArray.Exists(Ent.GetID()))
				{
					return nullptr;
				}
				return m_ComponentArray.Get(Ent.GetID());
			}

			/** Returns a pointer to a component tied to the input ID
			* Returns nullptr if the input ID doesn't have a component of type ComponentType tied to it
			@param ID
			@return ComponentType*
			*/
			ComponentType* GetComponent(unsigned int ID)
			{
				if (!m_ComponentArray.Exists(ID))
				{
					return nullptr;
				}
				return m_ComponentArray.Get(ID);
			}

			/** Returns a pointer to a const component tied to the input ID
			* Returns nullptr if the input ID doesn't have a component of type ComponentType tied to it
			@param ID
			@return const ComponentType*
			*/
			const ComponentType* GetComponent(unsigned int ID) const
			{
				if (!m_ComponentArray.Exists(ID))
				{
					return nullptr;
				}
				return m_ComponentArray.Get(ID);
			}

			/** Returns a reference to the internal sparse array containing all the components
			@return std::vector<ComponentType>&
			*/
			std::vector<ComponentType>& GetInternalArray()
			{
				return m_ComponentArray.GetArray();
			}

			/** Returns a const reference to the internal sparse array containing all the components
			@return const std::vector<ComponentType>&
			*/
			const std::vector<ComponentType>& GetInternalArray() const
			{
				return m_ComponentArray.GetArray();
			}

			/** Adds the input component to the input Entity.
			* Overwrites with the new component if the input Entity alread has a component of that type.
			* Fails softly if the input aZero::ECS::Entity has an invalid ID
			@param Ent
			@param Component
			@return void
			*/
			void AddComponent(Entity& Ent, ComponentType&& Component)
			{
				if (Ent.GetID() != std::numeric_limits<uint32_t>::max())
				{
					if (Ent.GetID() >= m_ComponentArray.GetArray().size())
					{
						m_ComponentArray.ForceExpand(m_AdditionalElementsWhenResize);
					}
					m_ComponentArray.Add(Ent.GetID(), std::forward<ComponentType>(Component));
				}
			}

			void AddComponent(Entity& Ent, ComponentType& Component)
			{
				if (Ent.GetID() != std::numeric_limits<uint32_t>::max())
				{
					if (Ent.GetID() >= m_ComponentArray.GetArray().size())
					{
						m_ComponentArray.ForceExpand(m_AdditionalElementsWhenResize);
					}
					m_ComponentArray.Add(Ent.GetID(), Component);
				}
			}

			/** Removes the component of the ComponentArray's type that is tied with the input Entity.
			* Fails softly if the input aZero::ECS::Entity doesn't have a component tied or is invalid
			@param Ent
			@return void
			*/
			void RemoveComponent(Entity& Ent)
			{
				m_ComponentArray.Remove(Ent.GetID());
			}

			/** Returns whether or not the input Entity has a component of the specific type.
			@param Ent
			@return bool
			*/
			bool HasComponent(const Entity& Ent)
			{
				return m_ComponentArray.Exists(Ent.GetID());
			}
		};
	}
}