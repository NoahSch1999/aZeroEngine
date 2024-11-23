/*
	IN PROGRESS / DEPRECATED
*/

//#pragma once
//#include <tuple>
//#include "System.h"
//
//namespace aZero
//{
//	namespace ECS
//	{
//		/** @brief Registers System subclasses for aZeroECS.
//		*/
//		template<typename ComponentManagerSpecialization, typename ...SystemTypes>
//		class SystemManager
//		{
//		private:
//			std::tuple<SystemTypes...> m_Systems;
//
//		public:
//			SystemManager() = default;
//
//			SystemManager(ComponentManagerSpecialization& ComponentManagerSpec)
//			{
//				std::apply([&ComponentManagerSpec](auto&&... args) {((args.SetComponentManager(ComponentManagerSpec)), ...); }, m_Systems);
//			}
//
//			template<typename SystemType>
//			SystemType& GetSystem()
//			{
//				return std::get<SystemType>(m_Systems);
//			}
//
//			void AddEntityToSystems(const Entity& entity)
//			{
//				for (auto& system : m_Systems)
//				{
//					system.Register(entity);
//				}
//			}
//
//			void RemoveEntityFromSystems(const Entity& entity)
//			{
//				for (auto& system : m_Systems)
//				{
//					system.UnRegister(entity);
//				}
//			}
//		};
//	}
//}