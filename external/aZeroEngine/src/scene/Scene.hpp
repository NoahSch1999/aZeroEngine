#pragma once
#include <bitset>
#include "SceneProxy.hpp"
#include "misc/HelperFunctions.hpp"
#include "misc/SparseSet.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneNew
		{
			ECS::Entity m_RootEntity;
		public:
			enum class ComponentFlag
			{
				Transform = 0,
				StaticMesh = 1,
				DirectionalLight = 2,
				PointLight = 3,
				SpotLight = 4,
				Camera = 5,

				All = 1337,
				None = 1338
			};

			SceneProxy* GetProxy() const { return m_Proxy.get(); }

			SceneNew()
				:m_Proxy(std::make_unique<SceneProxy>())
			{
				m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(1000);
				m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(1000);
				m_ComponentManager.GetComponentArray<ECS::PointLightComponent>().Init(1000);
				m_ComponentManager.GetComponentArray<ECS::SpotLightComponent>().Init(1000);
				m_ComponentManager.GetComponentArray<ECS::DirectionalLightComponent>().Init(1000);
				m_ComponentManager.GetComponentArray<ECS::CameraComponent>().Init(1000);

				m_RootEntity = m_EntityManager.CreateEntity();
				m_Entities["RootEntity"] = m_RootEntity;
				m_Entity_To_Name[m_RootEntity.GetID()] = "RootEntity";
				m_ComponentManager.AddComponent(m_RootEntity, aZero::ECS::TransformComponent(m_RootEntity));
			}

			ECS::Entity AddEntity()
			{
				ECS::Entity entity = m_EntityManager.CreateEntity();
				std::string newName = Helper::HandleNameCollision(this->GenerateEntityName(), m_Entities);
				m_Entities[newName] = entity;
				m_Entity_To_Name[entity.GetID()] = newName;
				m_ComponentManager.AddComponent(entity, aZero::ECS::TransformComponent(entity));
				
				this->ParentEntity(m_RootEntity, entity);

				return entity;
			}

			void RemoveEntity(const std::string& entity)
			{
				this->RemoveEntity(m_Entities.at(entity));
			}

			void RemoveEntity(const ECS::Entity& entity)
			{

			}

			template<typename ComponentType>
			void AddComponent(const ECS::Entity& entity, ComponentType&& component)
			{
				// Add the component to ecs
				
				// Remove the component if the entity is in m_DirtyEntities and the component is flagged in m_RemovedFlag
				
			}

			template<typename ComponentType>
			void RemoveComponent(const ECS::Entity& entity)
			{
				// Remove the component from ecs
				
				// Add entity to m_DirtyEntities and modify the component flag in m_RemovedFlag
				
			}

			template<typename ComponentType>
			void HasComponent(const ECS::Entity& entity) const
			{
				return m_ComponentManager.HasComponent<ComponentType>(entity);
			}

			void MarkRenderStateDirty(const ECS::Entity& entity, ComponentFlag flag)
			{
				ECS::TransformComponent* transformComp = m_ComponentManager.GetComponent<ECS::TransformComponent>(entity);
				DXM::Matrix entityTransform = transformComp->GetTransform();
				
				// TODO: Fix parenting transform calculations
				//DXM::Matrix entityTransform = DXM::Matrix::Identity;
				//if (transformComp)
				//{

				//	if (transformComp->m_ParentID.GetID() != ECS::InvalidEntityID)
				//	{
				//		// TODO: Go up parent chain and update transformComp accordinly
				//		ECS::TransformComponent* parentTransfrom = m_ComponentManager.GetComponent<ECS::TransformComponent>(transformComp->m_ParentID);
				//		entityTransform = parentTransfrom->GetTransform() * transformComp->GetTransform();

				//		for (auto& child : transformComp->m_ChildrenIDs)
				//		{
				//			this->MarkRenderStateDirty(child, ComponentFlag());
				//		}
				//	}
				//}

				ECS::EntityID id = entity.GetID();

				// NOTE: When we have SkeletalMeshComp we will make this if-statement be one or the other
				ECS::StaticMeshComponent* staticMeshComp = m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(entity);
				m_Proxy->UpdateStaticMesh(id, transformComp, staticMeshComp, entityTransform);

				ECS::PointLightComponent* pointLightComp = m_ComponentManager.GetComponent<ECS::PointLightComponent>(entity);
				m_Proxy->UpdatePointLight(id, pointLightComp, entityTransform);

				ECS::SpotLightComponent* spotLightComp = m_ComponentManager.GetComponent<ECS::SpotLightComponent>(entity);
				m_Proxy->UpdateSpotLight(id, spotLightComp, entityTransform);

				ECS::CameraComponent* cameraComp = m_ComponentManager.GetComponent<ECS::CameraComponent>(entity);
				m_Proxy->UpdateCamera(id, cameraComp, entityTransform);

				ECS::DirectionalLightComponent* DirLightComp = m_ComponentManager.GetComponent<ECS::DirectionalLightComponent>(entity);
				m_Proxy->UpdateDirectionalLight(id, DirLightComp);
			}

			void RenameEntity(ECS::Entity entity, const std::string& newName)
			{
				std::string entityName = Helper::HandleNameCollision(newName, m_Entities);
				m_Entities[entityName] = entity;
				m_Entities.erase(m_Entity_To_Name.at(entity.GetID()));
				m_Entity_To_Name[entity.GetID()] = entityName;
			}

			void ParentEntity(std::optional<ECS::Entity> parent, ECS::Entity child)
			{
				auto& arr = m_ComponentManager.GetComponentArray<ECS::TransformComponent>();
				ECS::TransformComponent* childComp = arr.GetComponent(child.GetID());
				ECS::TransformComponent* parentComp = arr.GetComponent(parent.value().GetID());

				if (!childComp || !parentComp)
					return;

				if (!parent.has_value())
				{
					ECS::TransformComponent nullComponent;
					childComp->SetParent(nullComponent);
				}

				ECS::Entity previousParent = childComp->GetParent();
				if (previousParent.GetID() != ECS::InvalidEntityID)
				{
					ECS::TransformComponent& prevParentComp = *arr.GetComponent(previousParent);
					prevParentComp.RemoveChild(*childComp);
				}

				parentComp->AddChild(*childComp);
			}

			std::optional<ECS::Entity> GetEntity(const std::string& name) { return m_Entities.count(name) == 1 ? m_Entities.at(name) : std::optional<ECS::Entity>{}; }
			const std::unordered_map<std::string, ECS::Entity>& GetEntities() const { return m_Entities; }

			ECS::ComponentManagerDecl m_ComponentManager;

			ECS::EntityManager m_EntityManager;
		private:
			std::string GenerateEntityName()
			{
				static uint32_t nameDummy = 0;
				return "Entity_" + std::to_string(nameDummy++);
			}

			/*void OnDestroy(){}
			void CreateRenderUpdates(){}
			void ClearRenderUpdates(){}*/

			// More mem than necessary, but quick add/remove + itteration
			/*DS::SparseSet<ECS::EntityID, ECS::EntityID> m_NewEntities;
			DS::SparseSet<ECS::EntityID, ECS::EntityID> m_RemovedEntities;

			struct ComponentUpdateInfo
			{
				std::bitset<sizeof(int32_t)> m_UpdateFlag;
				std::bitset<sizeof(int32_t)> m_RemovedFlag;
			};
			DS::SparseSet<ECS::EntityID, ComponentUpdateInfo> m_DirtyEntities;*/
			//

			std::unique_ptr<SceneProxy> m_Proxy;

			std::unordered_map<std::string, ECS::Entity> m_Entities;
			std::unordered_map<ECS::EntityID, std::string> m_Entity_To_Name;


		};
	}
}