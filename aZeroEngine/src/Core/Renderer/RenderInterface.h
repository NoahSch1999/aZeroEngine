#pragma once
#include "Renderer.h"
#include "Core/aZeroECSWrap/ComponentManagerInterface.h"

namespace aZero
{
	namespace Rendering
	{
		class RenderInterface
		{
		private:
			Renderer& m_Renderer;
			ECS::ComponentManagerDecl& m_ComponentManager;

			bool IsElegibleAsPrimitive(const ECS::Entity& Entity)
			{
				if (Entity.GetComponentMask().test(m_ComponentManager.GetComponentBit<ECS::TransformComponent>())
					&& Entity.GetComponentMask().test(m_ComponentManager.GetComponentBit<ECS::StaticMeshComponent>())
					)
				{
					return true;
				}

				return false; // check bitset
			}

			void UpdateFullPrimitive(SceneEntity& Entity)
			{
				const ECS::TransformComponent* TransformComp = m_ComponentManager.GetComponentArray<ECS::TransformComponent>().GetComponent(Entity.GetEntity());
				const ECS::StaticMeshComponent* StaticMeshComp = m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().GetComponent(Entity.GetEntity());
				
				PrimitiveRenderData PrimData;
				PrimData.WorldMatrix = TransformComp->GetTransform();
				if (StaticMeshComp->m_MeshReference.HasRenderState())
				{
					PrimData.MeshEntryIndex = StaticMeshComp->m_MeshReference.GetGPUAssetHandle()->m_MeshEntryAllocHandle.GetStartOffset() / sizeof(Asset::MeshGPUEntry);
				}
				else
				{
					PrimData.MeshEntryIndex = 0;
				}

				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				Scene->UpdatePrimitive(Entity.GetEntity().GetID(), &PrimData, m_Renderer.GetFrameAllocator());
			}

			void AddIfNotPrimitive(SceneEntity& Entity)
			{
				if (this->IsElegibleAsPrimitive(Entity.GetEntity()))
				{
					Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
					if (!Scene->IsPrimitive(Entity.GetEntity().GetID()))
					{
						Scene->AddPrimitive(Entity.GetEntity().GetID());
						//this->UpdateFullPrimitive(Entity);
					}
				}
			}

			void RemovePrimitiveIfAdded(SceneEntity& Entity)
			{
				if (!this->IsElegibleAsPrimitive(Entity.GetEntity()))
				{
					Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
					if (Scene->IsPrimitive(Entity.GetEntity().GetID()))
					{
						Scene->RemovePrimitive(Entity.GetEntity().GetID());
					}
				}
			}

			void AddIfNotPointLight(SceneEntity& Entity)
			{
				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				if (!Scene->IsPointLight(Entity.GetEntity().GetID()))
				{
					Scene->AddPointLight(Entity.GetEntity().GetID());
				}
			}

			void RemovePointLightIfAdded(SceneEntity& Entity)
			{
				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				if (Scene->IsPointLight(Entity.GetEntity().GetID()))
				{
					Scene->RemovePointLight(Entity.GetEntity().GetID());
				}
			}

			void UpdateFullRenderState(SceneEntity& Entity)
			{
				// Go through all components of value and call MarkRenderStateDirty on them
			}

		public:
			RenderInterface(Renderer& InRenderer, ECS::ComponentManagerDecl& InComponentManager)
				:m_Renderer(InRenderer), m_ComponentManager(InComponentManager)
			{

			}

			// TODO: Remove when we dont need to know the index count when doing a draw call
			std::unordered_map<uint32_t, Asset::Mesh>& GetEntityToMesh() { return m_Renderer.m_Entity_To_Mesh; }

			void MarkRenderStateDirty(Asset::Mesh& Mesh)
			{
				m_Renderer.MarkRenderStateDirty(Mesh);
			}

			void MarkRenderStateDirty(Asset::Material& Material)
			{
				m_Renderer.MarkRenderStateDirty(Material);
			}

			template<typename = void>
			void MarkRenderStateDirty(SceneEntity& Entity)
			{
				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				if (Scene)
				{
					this->AddIfNotPrimitive(Entity);
					this->AddIfNotPointLight(Entity);
					this->UpdateFullRenderState(Entity);
				}
			}

			template<>
			void MarkRenderStateDirty<ECS::TransformComponent>(SceneEntity& Entity)
			{
				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				if (Scene)
				{
					ECS::TransformComponent* Component = m_ComponentManager.GetComponent<ECS::TransformComponent>(Entity.GetEntity());
					if (Component)
					{
						this->AddIfNotPrimitive(Entity);

						Scene->UpdatePrimitive(
							Entity.GetEntity().GetID(),
							(void*)&Component->GetTransform(),
							m_Renderer.GetFrameAllocator(),
							PRIMITIVE_DATA_WORLDMATRIX_OFFSET,
							sizeof(ECS::TransformComponent));
					}
					else
					{
						this->RemovePrimitiveIfAdded(Entity);
					}
				}
			}

			template<>
			void MarkRenderStateDirty<ECS::StaticMeshComponent>(SceneEntity& Entity)
			{
				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				if (Scene)
				{
					ECS::StaticMeshComponent* Component = m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(Entity.GetEntity());
					if (Component)
					{
						this->AddIfNotPrimitive(Entity);

						uint32_t MeshIndex = 0; // TODO: Make MeshGPUEntry index 0 result in an 0 vert draw call
						if (Component->m_MeshReference.HasRenderState())
						{
							MeshIndex = Component->m_MeshReference.GetGPUAssetHandle()->m_MeshEntryAllocHandle.GetStartOffset() / sizeof(Asset::MeshGPUEntry);
						}

						uint32_t MaterialIndex = 0;
						if (Component->m_MaterialReference.HasRenderState())
						{
							MaterialIndex = Component->m_MaterialReference.GetGPUAssetHandle()->m_MaterialAllocHandle.GetStartOffset() / sizeof(Asset::MaterialRenderData);
						}
						

						Scene->UpdatePrimitive(
							Entity.GetEntity().GetID(),
							(void*)&MeshIndex,
							m_Renderer.GetFrameAllocator(),
							PRIMITIVE_DATA_MESHINDEX_OFFSET,
							sizeof(uint32_t));

						Scene->UpdatePrimitive(
							Entity.GetEntity().GetID(),
							(void*)&MaterialIndex,
							m_Renderer.GetFrameAllocator(),
							PRIMITIVE_DATA_MATERIALINDEX_OFFSET,
							sizeof(uint32_t));
					}
					else
					{
						this->RemovePrimitiveIfAdded(Entity);
					}
				}
			}

			template<>
			void MarkRenderStateDirty<ECS::PointLightComponent>(SceneEntity& Entity)
			{
				Rendering::RenderScene* Scene = Entity.GetScene().GetRenderScene();
				if (Scene)
				{
					ECS::PointLightComponent* Component = m_ComponentManager.GetComponent<ECS::PointLightComponent>(Entity.GetEntity());
					if (Component)
					{
						this->AddIfNotPointLight(Entity);

						PointLightRenderData RenderData;
						RenderData.Position = Component->GetPosition();
						RenderData.Color = Component->GetColor();
						RenderData.Range = Component->GetRange();

						if (m_ComponentManager.HasComponent<ECS::TransformComponent>(Entity.GetEntity()))
						{
							RenderData.Position += m_ComponentManager.GetComponentArray<ECS::TransformComponent>().GetComponent(Entity.GetEntity())->GetTransform().Translation();
						}

						Scene->UpdatePointLight(Entity.GetEntity().GetID(), &RenderData, m_Renderer.GetFrameAllocator());
					}
					else
					{
						this->RemovePointLightIfAdded(Entity);
					}
				}
			}
		};
	}
}