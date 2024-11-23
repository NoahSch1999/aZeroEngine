#pragma once
#include <string>

#include "Core/aZeroECSWrap/ComponentManagerInterface.h"

namespace aZero
{
	class Engine;

	class Scene
	{
	public:
		friend Engine;

	private:
		ECS::EntityManager* m_EntityManager = nullptr;

		std::string m_Name;

		std::unordered_map<std::string, ECS::Entity> m_Entities;

		void RemoveEntityComponents(ECS::Entity& Entity)
		{

			if (gComponentManager.HasComponent<ECS::TransformComponent>(Entity))
			{
				gComponentManager.RemoveComponent<ECS::TransformComponent>(Entity);
			}

			if (gComponentManager.HasComponent<ECS::StaticMeshComponent>(Entity))
			{
				gComponentManager.RemoveComponent<ECS::StaticMeshComponent>(Entity);
			}

			if (gComponentManager.HasComponent<TickComponent_Interface>(Entity))
			{
				gComponentManager.RemoveComponent<TickComponent_Interface>(Entity);
			}
		}

		void Init(const std::string& Name, ECS::EntityManager& EntityManager)
		{
			m_Name = Name;
			m_EntityManager = &EntityManager;
		}

	public:
		Scene() = default;

		Scene(const std::string& Name, ECS::EntityManager& EntityManager)
		{
			this->Init(Name, EntityManager);
		}

		void ClearEntities()
		{
			for (auto& Entity : m_Entities)
			{
				this->RemoveEntityComponents(Entity.second);
			}
			m_Entities.clear();
		}

		ECS::Entity& CreateEntity(const std::string& Name)
		{
			if (m_Entities.count(Name))
			{
				// Handle duplicate name
				throw;
				//
			}
			
			m_Entities.emplace(Name, m_EntityManager->CreateEntity());
			ECS::Entity& Entity = m_Entities.at(Name);

			return Entity;
		}

		void RemoveEntity(const std::string& Name)
		{
			if (m_Entities.count(Name))
			{
				ECS::Entity& Entity = m_Entities.at(Name);

				this->RemoveEntityComponents(Entity);
				m_EntityManager->RemoveEntity(Entity);

				m_Entities.erase(Name); // NOTE : Goes wrong...
			}
		}

		const std::string& GetName() const { return m_Name; }

		const std::unordered_map<std::string, ECS::Entity>& GetEntityMap() const { return m_Entities; }
	};
}