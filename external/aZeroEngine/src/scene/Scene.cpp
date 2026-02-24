#include "Scene.hpp"

namespace aZero
{
	namespace Scene
	{
		Scene::Scene(SceneID ID, const std::string& name)
		{
			m_ID = ID;
			m_Name = name;

			constexpr uint32_t MaxEntities = 1000;
			m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(MaxEntities);
			m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MaxEntities);
			m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MaxEntities);
			m_ComponentManager.GetComponentArray<ECS::DirectionalLightComponent>().Init(MaxEntities);
			m_ComponentManager.GetComponentArray<ECS::PointLightComponent>().Init(MaxEntities);
			m_ComponentManager.GetComponentArray<ECS::SpotLightComponent>().Init(MaxEntities);
			m_ComponentManager.GetComponentArray<ECS::CameraComponent>().Init(MaxEntities);
		}

		void Scene::RenameEntity(ECS::Entity entity, const std::string& newName)
		{
			std::string entityName = Helper::HandleNameCollision(newName, m_Entities);
			m_Entities[entityName] = entity;
			m_Entities.erase(m_Entity_To_Name.at(entity.GetID()));
			m_Entity_To_Name[entity.GetID()] = entityName;
		}

		ECS::Entity Scene::CreateEntity()
		{
			ECS::Entity entity = m_EntityManager.CreateEntity();
			std::string newName = Helper::HandleNameCollision(this->GenerateEntityName(), m_Entities);
			m_Entities[newName] = entity;
			m_Entity_To_Name[entity.GetID()] = newName;
			m_ComponentManager.AddComponent(entity, ECS::TransformComponent());
			return entity;
		}

		void Scene::DeleteEntity(const std::string& name)
		{
			if (m_Entities.count(name))
			{
				this->DeleteEntity(m_Entities.at(name));
			}
		}

		void Scene::DeleteEntity(ECS::Entity entity)
		{
			if (m_Entity_To_Name.count(entity.GetID()))
			{
				std::apply([&entity](auto& ... args)
					{
						(args.RemoveComponent(entity), ...);
					}, m_ComponentManager.GetComponentTuple()
						);

				this->UpdateRenderState(entity);

				m_Entities.erase(m_Entity_To_Name.at(entity.GetID()));
				m_Entity_To_Name.erase(entity.GetID());
			}
		}

		void Scene::UpdateRenderState(ECS::Entity entity)
		{
			const ECS::EntityID id = entity.GetID();
			if (ECS::TransformComponent* tf = m_ComponentManager.GetComponent<ECS::TransformComponent>(entity))
			{
				const ECS::StaticMeshComponent* mesh = m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(entity);
				m_Proxy.UpdateStaticMesh(id, tf, mesh);

				const ECS::CameraComponent* camera = m_ComponentManager.GetComponent<ECS::CameraComponent>(entity);
				m_Proxy.UpdateCamera(id, camera);

				const ECS::DirectionalLightComponent* directionalLight = m_ComponentManager.GetComponent<ECS::DirectionalLightComponent>(entity);
				m_Proxy.UpdateDirectionalLight(id, directionalLight);

				const ECS::PointLightComponent* pointLight = m_ComponentManager.GetComponent<ECS::PointLightComponent>(entity);
				m_Proxy.UpdatePointLight(id, pointLight);

				const ECS::SpotLightComponent* spotLight = m_ComponentManager.GetComponent<ECS::SpotLightComponent>(entity);
				m_Proxy.UpdateSpotLight(id, spotLight);
			}
			else
			{
				m_Proxy.UpdateStaticMesh(id, nullptr, nullptr);
				m_Proxy.UpdateCamera(id, nullptr);
				m_Proxy.UpdateDirectionalLight(id, nullptr);
				m_Proxy.UpdatePointLight(id, nullptr);
				m_Proxy.UpdateSpotLight(id, nullptr);
			}
		}
	}
}