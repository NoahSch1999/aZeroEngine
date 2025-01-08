#pragma once
#include <string>
#include <Core/aZeroECSWrap/aZeroECS/aZeroECS.h>
#include "Core/Renderer/D3D12Wrap/Resources/LinearBuffer.h"

namespace aZero
{
	class Engine;
	class Scene;

	namespace Rendering
	{
		class Renderer;
	}

	namespace NewScene
	{
		using SceneID = uint16_t;
		constexpr SceneID InvalidSceneID = std::numeric_limits<SceneID>::max();

		class SceneEntity
		{
		private:
			ECS::Entity m_Entity;
			SceneID m_SceneID = InvalidSceneID;

			void MoveOp(SceneEntity& Other)
			{
				m_Entity = Other.m_Entity;
				m_SceneID = Other.m_SceneID;
				Other.m_SceneID = InvalidSceneID;
			}

		public:
			SceneEntity(const SceneEntity&) = delete;
			SceneEntity& operator=(const SceneEntity&) = delete;

			SceneEntity() = default;

			SceneEntity(ECS::Entity Entity, SceneID ID)
				:m_Entity(Entity), m_SceneID(ID)
			{

			}

			SceneEntity(SceneEntity&& Other) noexcept
			{
				this->MoveOp(Other);
			}

			SceneEntity& operator=(SceneEntity&& Other) noexcept
			{
				if (this != &Other)
				{
					this->MoveOp(Other);
				}
				return *this;
			}

			// TODO: Remove and make scene a friend instead
			ECS::Entity& GetEntity() { return m_Entity; }
			const ECS::Entity& GetEntity() const { return m_Entity; }

			ECS::EntityID GetEntityID() const { return m_Entity.GetID(); }
			SceneID GetSceneID() const { return m_SceneID; }
		};

		class Scene
		{
			friend Rendering::Renderer;
		public:
			struct StaticMesh
			{
				DXM::Matrix WorldMatrix;
				std::shared_ptr<Asset::Mesh> Mesh;
				std::shared_ptr<Asset::Material> Material;
				float Bounds;
			};

			struct PointLight
			{
				DXM::Vector3 Position;
				DXM::Vector3 Color;
				float Range;
			};

			struct Camera
			{
				bool IsActive = false;
				D3D12_VIEWPORT Viewport;
				D3D12_RECT ScizzorRect;
				DXM::Matrix ViewProjectionMatrix;
				// Frustrum
			};

			// TODO: Potentially replace with SparseLookupArray, or the other way around
			//		If we decide to use ENTT we scrap SparseLookupArray fully
			template<typename PrimitiveType>
			class PackedVector
			{
				friend Rendering::Renderer;
			private:
				std::unordered_map<ECS::EntityID, uint32_t> m_Entity_To_Index;
				std::unordered_map<uint32_t, ECS::EntityID> m_Index_To_Entity;
				std::vector<PrimitiveType> m_Data;
				uint32_t m_CurrentLastIndex = 0;

				// TODO: These need to be adapted based on if we copy indices or the actual instance data before rendering
				// Regardless, they need to be resizable if their extends are reached
				std::vector<D3D12::LinearBuffer> m_RenderBuffers;

			public:
				using PrimType = PrimitiveType;

				PackedVector() = default;

				// TODO: Optimize / make more smooth
				void AddOrUpdate(ECS::EntityID EntityID, PrimitiveType&& Primitive)
				{
					if (auto Index = m_Entity_To_Index.find(EntityID); Index != m_Entity_To_Index.end())
					{
						m_Data.at(Index->second) = Primitive;
					}
					else
					{
						m_Entity_To_Index[EntityID] = m_CurrentLastIndex;
						m_Index_To_Entity[m_CurrentLastIndex] = EntityID;

						if (m_Data.size() > m_CurrentLastIndex)
						{
							m_Data.at(m_CurrentLastIndex) = Primitive;
						}
						else
						{
							m_Data.emplace_back(Primitive);
						}
						m_CurrentLastIndex++;
					}
				}

				// TODO: Optimize / make more smooth
				void Remove(ECS::EntityID EntityID)
				{
					if (auto Index = m_Entity_To_Index.find(EntityID); Index != m_Entity_To_Index.end())
					{
						if (Index->second != m_CurrentLastIndex)
						{
							// Copy last element index data to removed index data
							const uint32_t LastElementIndex = m_CurrentLastIndex - 1;
							m_Data[Index->second] = std::move(m_Data.at(LastElementIndex));

							m_Data.at(LastElementIndex) = PrimitiveType(); // Reset
							
							// Remap entity=>index map so moved entityid points to removed index
							const ECS::EntityID LastEntity = m_Index_To_Entity.at(LastElementIndex);
							m_Entity_To_Index.at(LastEntity) = Index->second;
							
							// Remap so that index=>entity map so removed index points to last entity
							m_Index_To_Entity.at(Index->second) = LastEntity;

							// Remove old entity id from entity=>index map
							m_Entity_To_Index.erase(EntityID);

							// Remove last index from index=>entity
							m_Index_To_Entity.erase(LastElementIndex);
						}
						else
						{
							m_Data.at(Index->second) = PrimitiveType(); // Reset
						}

						m_CurrentLastIndex--;
					}
				}

				void ShrinkToFit()
				{
					if (m_CurrentLastIndex)
					{
						m_Data.resize(m_CurrentLastIndex);
					}
				}

				const std::vector<PrimitiveType>& GetData() const { return m_Data; }
			};

			template<typename ...Args>
			class RenderProxyClass
			{
				friend Rendering::Renderer;
			private:
				std::tuple<PackedVector<Args>...> m_PrimitiveTuple;

				void MoveOp(RenderProxyClass& Other)
				{
					m_PrimitiveTuple = std::move(m_PrimitiveTuple);
				}

				template<typename PrimitiveType>
				PackedVector<PrimitiveType>& GetPrimitives()
				{
					return std::get<PackedVector<PrimitiveType>>(m_PrimitiveTuple);
				}

			public:

				RenderProxyClass(const RenderProxyClass&) = delete;
				RenderProxyClass& operator=(const RenderProxyClass&) = delete;

				RenderProxyClass() = default;

				RenderProxyClass(RenderProxyClass&& Other) noexcept
				{
					this->MoveOp(Other);
				}

				RenderProxyClass& operator=(RenderProxyClass&& Other) noexcept
				{
					if (this != &Other)
					{
						this->MoveOp(Other);
					}
					return *this;
				}

				template<typename PrimitiveType>
				void AddOrUpdatePrimitive(ECS::EntityID EntityID, PrimitiveType&& Primitive)
				{
					std::get<PackedVector<PrimitiveType>>(m_PrimitiveTuple).AddOrUpdate(EntityID, std::forward<PrimitiveType>(Primitive));
				}

				template<typename PrimitiveType>
				void RemovePrimitive(ECS::EntityID EntityID)
				{
					std::get<PackedVector<PrimitiveType>>(m_PrimitiveTuple).Remove(EntityID);
				}

				template<typename PrimitiveType>
				const PackedVector<PrimitiveType>& GetPrimitives() const
				{
					return std::get<PackedVector<PrimitiveType>>(m_PrimitiveTuple);
				}

				std::tuple<PackedVector<Args>...>& GetPrimitiveTuple() { return m_PrimitiveTuple; }
			};

			using RenderProxy = RenderProxyClass<Scene::StaticMesh, Scene::PointLight, Scene::Camera>;

		private:
			SceneID m_ID;
			ECS::EntityManager m_EntityManager;
			ECS::ComponentManagerDecl m_ComponentManager;

			std::unordered_map<std::string, SceneEntity> m_Entities;

			RenderProxy m_RenderProxy;

			// TODO: Test move op with compmanager
			void MoveOp(Scene& Other)
			{
				m_ID = Other.m_ID;
				m_EntityManager = std::move(m_EntityManager);
				m_ComponentManager = std::move(m_ComponentManager);
				m_Entities = std::move(m_Entities);
				m_RenderProxy = std::move(Other.m_RenderProxy);

				Other.m_ID = InvalidSceneID;
			}

		public:

			Scene(const Scene&) = delete;
			Scene& operator=(const Scene&) = delete;

			Scene(SceneID ID, ID3D12Device* Device)
				:m_ID(ID)
			{
				// TODO: Remove the need to init all new component type
				constexpr uint32_t MaxEntities = 1000;
				m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::PointLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::CameraComponent>().Init(MaxEntities);
				//
			}

			~Scene()
			{
				if (m_ID != InvalidSceneID)
				{
					// TODO: Impl
				}
			}

			Scene(Scene&& Other) noexcept
			{
				this->MoveOp(Other);
			}

			Scene& operator=(Scene&& Other) noexcept
			{
				if (this != &Other)
				{
					this->MoveOp(Other);
				}
				return *this;
			}

			SceneEntity* CreateEntity(const std::string& Name)
			{
				std::string NewName = Name;
				uint32_t AddNum = 0;
				if(m_Entities.count(Name) > 0)
				{
					std::string Temp = NewName + "_" + std::to_string(AddNum);
					while(m_Entities.count(Temp) > 0)
					{
						Temp = NewName + "_" + std::to_string(AddNum);
						AddNum++;
					}
					NewName = Temp;
				}

				return &(m_Entities[NewName] = std::move(SceneEntity(m_EntityManager.CreateEntity(), m_ID)));
			}

			// TODO: Add function which takes a SceneEntity (some name -> entity map is needed)
			void RemoveEntity(const std::string& Name)
			{
				if (auto Entity = m_Entities.find(Name); Entity != m_Entities.end())
				{
					if (Entity->second.GetSceneID() != m_ID)
					{
						return;
					}

					ECS::EntityID ID = Entity->second.GetEntityID();
					std::apply([ID](auto&& ... args)
						{
							(args.Remove(ID), ...);
						}, m_RenderProxy.GetPrimitiveTuple()
					);

					std::apply([Entity](auto&& ... args)
						{
							(args.RemoveComponent(Entity->second.GetEntity()), ...);
						}, m_ComponentManager.GetComponentTuple()
						);

					m_Entities.erase(Name);
				}
			}

			SceneEntity* GetEntity(const std::string& Name)
			{
				if (auto Entity = m_Entities.find(Name); Entity != m_Entities.end())
				{
					return &Entity->second;
				}

				return nullptr;
			}

			template<typename Component>
			void AddComponent(SceneEntity& Entity, Component&& InComponent)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return;
				}

				m_ComponentManager.AddComponent<Component>(Entity.GetEntity(), std::forward<Component>(InComponent));
			}

			template<typename Component>
			void RemoveComponent(SceneEntity& Entity)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return;
				}

				// Fails silently if no component exists
				m_ComponentManager.RemoveComponent<Component>(Entity.GetEntity());
			}

			template<typename Component>
			Component* GetComponent(const SceneEntity& Entity)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return nullptr;
				}

				// Fails silently and returns nullptr if no component exists
				return m_ComponentManager.GetComponent<Component>(Entity.GetEntity());
			}

			template<typename PrimitiveType>
			const std::vector<PrimitiveType>& GetPrimitives() const
			{
				return m_RenderProxy.GetPrimitives<PrimitiveType>().GetData();
			}

			void ShrinkPrimitiveArrays()
			{
				std::apply([](auto&& ... args)
					{
						(args.ShrinkToFit(), ...);
					}, m_RenderProxy.GetPrimitiveTuple()
						);
			}

			void MarkRenderStateDirty(const SceneEntity& Entity)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return;
				}

				// TODO: Insert into tree etc if entity is supposed to be (maybe SceneEntity has a movable flag which decides)

				// TODO: Handle parenting here?

				// TODO: Handle resize of relevant gpu buffers

				ECS::TransformComponent* TransformComp = m_ComponentManager.GetComponent<ECS::TransformComponent>(Entity.GetEntity());
				ECS::StaticMeshComponent* StaticMeshComp = m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(Entity.GetEntity());

				// NOTE: When we have SkeletalMeshComp we will make this if-statement be one or the other
				if (TransformComp && StaticMeshComp)
				{
					// TODO: If we send indices instead of the whole prim:
					/*
						1. Copy primitive data into upload heap and enqueue copy to gpu-only buffer
						2. StaticMesh should have an index to the gpu-only buffer index, the bounds and mesh/mat info
						3. When performing culling, we use bounds to cull and then copy id to per-render instance buffer containing all ids
						4. In shader, we use id to get per-instance data
					*/

					StaticMesh MeshPrim;
					const DXM::Matrix WorldMatrix = TransformComp->GetTransform();
					MeshPrim.WorldMatrix = WorldMatrix;

					if (StaticMeshComp->m_MeshReference && StaticMeshComp->m_MaterialReference)
					{
						const float XAxisScale = WorldMatrix.m[0][0];
						const float YAxisScale = WorldMatrix.m[1][1];
						const float ZAxisScale = WorldMatrix.m[2][2];
						const float MaxScaleAxis = std::max(abs(XAxisScale), std::max(abs(YAxisScale), abs(ZAxisScale)));
						MeshPrim.Bounds = StaticMeshComp->m_MeshReference->GetData().BoundingSphereRadius * MaxScaleAxis;

						// TODO: This should be changed to avoid using pointers because its gonna be slow af when batching...
						/*
							Current problem is we need refcount to the meshes used.
							This can be moved so we keep a map with primitive -> mesh/mat instead to avoid fetching ptr when culling
							But we also need the MeshEntryIndex, MaterialIndex and num indices when drawing...
							MeshEntryIndex and MaterialIndex can be fetched once and stored in the per-prim data.
							However... the difficult part is the num indices data.
								Since the asset might change, we need to reflect that on the per-prim data...
								HOW?
								This will easily be solved once we do indirect drawing using MeshEntry which contains num indices...
								But for now...maybe just not allow setting data if the new num indices are not the same?
						*/
						MeshPrim.Mesh = StaticMeshComp->m_MeshReference;
						MeshPrim.Material = StaticMeshComp->m_MaterialReference;

						m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(MeshPrim));
					}
				}

				ECS::PointLightComponent* PointLightComp = m_ComponentManager.GetComponent<ECS::PointLightComponent>(Entity.GetEntity());
				if (PointLightComp)
				{
					PointLight PLPrim;
					PLPrim.Color = PointLightComp->GetColor();

					if (TransformComp)
					{
						PLPrim.Position = DXM::Vector3::Transform(PointLightComp->GetPosition(), TransformComp->GetTransform());
					}
					else
					{
						PLPrim.Position = PointLightComp->GetPosition();
					}

					PLPrim.Range = PointLightComp->GetRange();
					m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(PLPrim));
				}

				ECS::CameraComponent* CameraComp = m_ComponentManager.GetComponent<ECS::CameraComponent>(Entity.GetEntity());
				if (CameraComp)
				{
					// TODO: Multiply camera position with tf position?
					Camera CamPrim;
					CamPrim.IsActive = true;
					CamPrim.Viewport = CameraComp->GetViewport();
					CamPrim.ScizzorRect = CameraComp->GetScizzorRect();
					CamPrim.ViewProjectionMatrix = CameraComp->GetViewProjectionMatrix();
					m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(CamPrim));
				}
			}

			void MarkRenderStateDirty(const std::string& EntityName)
			{
				if (auto Entity = m_Entities.find(EntityName); Entity != m_Entities.end())
				{
					this->MarkRenderStateDirty(Entity->second);
				}
			}
		};
	}
}