#pragma once
#include <tuple>
#include "ComponentArray.h"

namespace aZero
{
	namespace ECS
	{
		template<typename TargetType, typename... TupleTypes>
		constexpr int GetIndexOfTupleElement(const std::tuple<TupleTypes...>& Tuple)
		{
			bool found = false;
			int count = 0;

			((!found ? (++count, found = std::is_same_v<TargetType, TupleTypes>) : 0), ...);

			return found ? count - 1 : throw;
		}

		/** @brief A class storing all the types of aZero::ECS::ComponentArray
		*/
		template<class... Args>
		class ComponentManager
		{
			std::tuple<ComponentArray<Args>...> m_ComponentArrays;

		public:
			ComponentManager() = default;
			~ComponentManager() = default;

			/** Returns the bit index of the component type specified
			@return int Index
			*/
			template<typename ComponentType>
			constexpr int GetComponentBit() const
			{
				return GetIndexOfTupleElement<ComponentArray<ComponentType>>(m_ComponentArrays);
			}

			/** Returns the aZero::ECS::ComponentArray of the specified type
			@return aZero::ECS::ComponentArray<ComponentType>& Reference to the array
			*/
			template<typename ComponentType>
			ComponentArray<ComponentType>& GetComponentArray()
			{
				return std::get<ComponentArray<ComponentType>>(m_ComponentArrays);
			}

			/** Adds a component for the input aZero::ECS::Entity with the ComponentType default constructor
			* Sets the component bit
			@param Ent
			@return void
			*/
			template<typename ComponentType>
			void AddComponent(Entity& Ent)
			{
				GetComponentArray<ComponentType>().AddComponent(Ent, ComponentType());
				Ent.SetComponentBit(GetComponentBit<ComponentType>(), true);
			}

			/** Adds a component for the input aZero::ECS::Entity and initializes with the input ComponentType data
			* Sets the component bit
			@param Ent
			@param Component
			@return void
			*/
			template<typename ComponentType>
			void AddComponent(Entity& Ent, ComponentType&& Component)
			{
				GetComponentArray<ComponentType>().AddComponent(Ent, std::forward<ComponentType>(Component));
				Ent.SetComponentBit(GetComponentBit<ComponentType>(), true);
			}

			/** Removes a component for the input aZero::ECS::Entity
			* Disables the component bit
			* Fails softly if the aZero::ECS::Entity doesn't have a component of type ComponentType
			@param Ent
			@return void
			*/
			template<typename ComponentType>
			void RemoveComponent(Entity& Ent)
			{
				GetComponentArray<ComponentType>().RemoveComponent(Ent);
				Ent.SetComponentBit(GetComponentBit<ComponentType>(), false);
			}

			/** Checks the bitmask of aZero::ECS::Entity and returns true if the aZero::ECS::Entity has a component of type ComponentType
			@param Ent
			@return bool
			*/
			template<typename ComponentType>
			bool HasComponent(const Entity& Ent) const
			{
				return Ent.GetComponentMask().test(GetComponentBit<ComponentType>());
			}
		};
	}
}