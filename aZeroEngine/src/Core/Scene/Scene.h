#pragma once
#include <string>

#include "Core/aZeroECSWrap/ComponentManagerInterface.h"

namespace aZero
{
	class Engine;
	class Scene;

	class SceneEntity
	{
		friend Scene;
	private:
		ECS::Entity m_Entity;
		Scene& m_Owner;

		SceneEntity(ECS::Entity Entity, Scene& OwningScene)
			:m_Entity(Entity), m_Owner(OwningScene)
		{

		}

	public:

		ECS::Entity& GetEntity()
		{
			return m_Entity;
		}

		const ECS::Entity& GetEntity() const
		{
			return m_Entity;
		}

		Scene& GetScene()
		{
			return m_Owner;
		}

		const Scene& GetScene() const
		{
			return m_Owner;
		}
	};

	class Scene
	{
	public:
		friend Engine;

	private:
		ECS::EntityManager& m_EntityManager;
		ECS::ComponentManagerDecl& m_ComponentManager;

		Rendering::RenderScene* m_RenderScene;

		std::string m_Name;

		std::unordered_map<std::string, SceneEntity> m_Entities;

		void RemoveEntityComponents(ECS::Entity& Entity)
		{

			if (m_ComponentManager.HasComponent<ECS::TransformComponent>(Entity))
			{
				m_ComponentManager.RemoveComponent<ECS::TransformComponent>(Entity);
			}

			if (m_ComponentManager.HasComponent<ECS::StaticMeshComponent>(Entity))
			{
				m_ComponentManager.RemoveComponent<ECS::StaticMeshComponent>(Entity);
			}

			if (m_ComponentManager.HasComponent<TickComponent_Interface>(Entity))
			{
				m_ComponentManager.RemoveComponent<TickComponent_Interface>(Entity);
			}
		}

		Scene(const std::string& Name, Rendering::RenderScene& RenderScene, ECS::EntityManager& EntityManager, ECS::ComponentManagerDecl& ComponentManager)
			:m_RenderScene(&RenderScene), m_EntityManager(EntityManager), m_ComponentManager(ComponentManager)
		{
			m_Name = Name;
		}

	public:

		void ClearEntities()
		{
			for (auto& Entity : m_Entities)
			{
				this->RemoveEntityComponents(Entity.second.GetEntity());
			}
			m_Entities.clear();
		}

		SceneEntity& CreateEntity(const std::string& Name)
		{
			if (m_Entities.count(Name))
			{
				// TODO: Handle duplicate name
				throw;
				//
			}
			
			m_Entities.emplace(Name, SceneEntity(m_EntityManager.CreateEntity(), *this));
			SceneEntity& Entity = m_Entities.at(Name);

			return Entity;
		}

		void RemoveEntity(const std::string& Name)
		{
			if (m_Entities.count(Name))
			{
				SceneEntity& Entity = m_Entities.at(Name);

				this->RemoveEntityComponents(Entity.GetEntity());
				m_EntityManager.RemoveEntity(Entity.GetEntity());

				m_Entities.erase(Name); // NOTE : Goes wrong...
			}
		}

		const std::string& GetName() const { return m_Name; }

		const std::unordered_map<std::string, SceneEntity>& GetEntityMap() const { return m_Entities; }

		Rendering::RenderScene* GetRenderScene() { return m_RenderScene; }

		const Rendering::RenderScene* GetRenderScene() const { return m_RenderScene; }
	};
}