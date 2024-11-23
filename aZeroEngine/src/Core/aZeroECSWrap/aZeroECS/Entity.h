#pragma once
#include <Windows.h>
#include <tuple>
#include <bitset>

namespace aZero
{
	namespace ECS
	{
		/** @brief Max number of components that the ECS supports */
		constexpr int MAX_COMPONENT_COUNT = 32;

		typedef unsigned int EntityID;

		/** @brief The Entity of the ECS */
		class Entity
		{
			friend class EntityManager;

			template<class... Args>
			friend class ComponentManager;

			template<typename ComponentType>
			friend class ComponentArray;

		private:
			EntityID m_ID = UINT_MAX;
			std::bitset<MAX_COMPONENT_COUNT> m_ComponentMask;

		private:

			void SetComponentBit(int ComponentBit, bool Value) { m_ComponentMask.set(ComponentBit, Value); }

		public:
			Entity() = default;

			/** Returns the unique ID of the Entity
			@return EntityID
			*/
			EntityID GetID() const { return m_ID; }

			/** Returns the component bitmask which describes what components the Entity currently has
			@return  std::bitset<MAX_COMPONENT_COUNT>&
			*/
			const std::bitset<MAX_COMPONENT_COUNT>& GetComponentMask() const { return m_ComponentMask; }
		};
	}
}