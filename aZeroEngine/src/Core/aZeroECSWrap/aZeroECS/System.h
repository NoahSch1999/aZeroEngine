/*
	IN PROGRESS / DEPRECATED
*/

//#pragma once
//#include "Entity.h"
//#include "PackedLookupArray.h"

//namespace aZero
//{
//	namespace ECS
//	{
//		template<typename ComponentManagerSpecialization>
//		class System
//		{
//		protected:
//			std::bitset<MAX_COMPONENT_COUNT> m_componentMask;
//			DataStructures::PackedLookupArray<int, Entity> m_entities;
//			ComponentManagerSpecialization* m_componentManager = nullptr;
//
//		public:
//			System() = default;
//
//			// TODO - make so only the system manage can access and set m_componentManager
//			void SetComponentManager(ComponentManagerSpecialization& InComponentManager)
//			{
//				m_componentManager = &InComponentManager;
//			}
//
//			virtual ~System() = default;
//
//			/** Pure virtual method which should be overridden by the inheriting subclass.
//			* Defines custom behavior related to the inheriting System when registering an Entity.
//			@return void
//			*/
//			virtual void OnRegister() = 0;
//
//			/** Pure virtual method which should be overridden by the inheriting subclass.
//			* Defines custom behavior related to the inheriting System when unregistering an Entity.
//			@return void
//			*/
//			virtual void OnUnRegister() = 0;
//			
//			/** Pure virtual method which should be overridden by the inheriting subclass.
//			@return void
//			*/
//			virtual void Update() = 0;
//
//			/** Registers the input Entity if it's m_componentMask matches the m_componentMask set in the System subclass constructor.
//			@param entity The Entity which should be bound
//			@return bool TRUE: Entity is either already bound or bound during this call of System::Register(). FALSE: Entity::m_componentMask isn't compatible with System::m_componentMask
//			*/
//			bool Register(const Entity& entity)
//			{
//				if (!IsRegistered(entity))
//				{
//					std::bitset<MAX_COMPONENT_COUNT> bitwiseResult = m_componentMask & entity.GetComponentMask();
//					if (bitwiseResult != m_componentMask)
//						return false;
//
//					m_entities.AddCopy(entity.GetID(), entity);
//
//					OnRegister();
//
//					return true;
//				}
//
//				return false;
//			}
//
//			/** Unregisters the input Entity.
//			@param entity The Entity which should be unbound
//			@return void
//			*/
//			void UnRegister(const Entity& entity)
//			{
//				if (IsRegistered(entity))
//				{
//					OnUnRegister();
//
//					m_entities.Remove(entity.GetID());
//				}
//			}
//
//			bool IsRegistered(const Entity& entity)
//			{
//				return m_entities.Exists(entity.GetID());
//			}
//
//			int NumEntitiesBound() const { return m_entities.GetNumElements(); }
//		};
//	}
}