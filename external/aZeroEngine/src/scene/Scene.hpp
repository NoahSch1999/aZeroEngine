#pragma once
#include <string>
#include <span>

#include "ecs/aZeroECS.hpp"
#include "graphics_api/resource_type/LinearBuffer.hpp"

namespace aZero
{
	class Engine;

	namespace Rendering
	{
		struct StaticMeshBatches
		{
			struct PerInstanceData
			{
				DXM::Matrix WorldMatrix;
			};

			struct BatchInfo
			{
				uint32_t StartInstanceOffset;
				uint32_t NumVertices;
				std::vector<PerInstanceData> InstanceData;
			};

			std::unordered_map<uint32_t, std::unordered_map<uint32_t, BatchInfo>> Batches;
		};

		class Renderer;
	}

	namespace Scene
	{
		class Scene;
		using SceneID = uint16_t;
		constexpr SceneID InvalidSceneID = std::numeric_limits<SceneID>::max();

		class SceneEntity : public NonCopyable
		{
			friend aZero::Scene::Scene;
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

			ECS::EntityID GetEntityID() const { return m_Entity.GetID(); }
			SceneID GetSceneID() const { return m_SceneID; }
		};

		class Scene : public NonCopyable
		{
			friend Rendering::Renderer;
		public:
			struct StaticMesh
			{
				DXM::Matrix WorldMatrix;
				std::shared_ptr<Asset::Mesh> Mesh;
				std::shared_ptr<Asset::Material> Material;
				float Bounds;
				DirectX::BoundingSphere DXBounds;
			};

			struct Camera
			{
				bool IsActive = false;
				D3D12_VIEWPORT Viewport;
				D3D12_RECT ScizzorRect;
				DXM::Matrix ViewMatrix;
				DXM::Matrix ProjMatrix;
			};

			// TODO: Potentially replace with SparseLookupArray, or the other way around
			//		If we decide to use ENTT we scrap SparseLookupArray fully
			template<typename ObjectType>
			class PackedVector
			{
				friend Rendering::Renderer;
			private:
				std::unordered_map<ECS::EntityID, uint32_t> m_Entity_To_Index;
				std::unordered_map<uint32_t, ECS::EntityID> m_Index_To_Entity;
				std::vector<ObjectType> m_Data;
				uint32_t m_CurrentLastIndex = 0;

				// TODO: Check performance diff if we copy the indices instead and keep the actual instance data on the gpu the entire time
				//  If we send indices instead of the whole prim:
					/*
						1. Copy primitive data into upload heap and enqueue copy to gpu-only buffer
						2. StaticMesh should have an index to the gpu-only buffer index, the bounds and mesh/mat info
						3. When performing culling, we use bounds to cull and then copy id to per-render instance buffer containing all ids
						4. In shader, we use id to get per-instance data
					*/
				std::vector<D3D12::LinearBuffer> m_RenderBuffers;

			public:
				using PrimType = ObjectType;

				PackedVector() = default;

				void AddOrUpdate(ECS::EntityID EntityID, ObjectType&& Primitive)
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

				void Remove(ECS::EntityID EntityID)
				{
					if (auto Index = m_Entity_To_Index.find(EntityID); Index != m_Entity_To_Index.end())
					{
						if (Index->second != m_CurrentLastIndex)
						{
							// Copy last element index data to removed index data
							const uint32_t LastElementIndex = m_CurrentLastIndex - 1;
							m_Data[Index->second] = std::move(m_Data.at(LastElementIndex));

							m_Data.at(LastElementIndex) = ObjectType(); // Reset
							
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
							m_Data.at(Index->second) = ObjectType(); // Reset
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

				const std::span<const ObjectType> GetData() const { return std::span{ m_Data }; }
			};

			template<typename ...Args>
			class RenderProxyClass : public NonCopyable
			{
				friend Rendering::Renderer;
			private:
				std::tuple<PackedVector<Args>...> m_ObjectTuple;

				void MoveOp(RenderProxyClass& Other) noexcept
				{
					m_ObjectTuple = std::move(Other.m_ObjectTuple);
				}

				template<typename ObjectType>
				PackedVector<ObjectType>& GetObjects()
				{
					return std::get<PackedVector<ObjectType>>(m_ObjectTuple);
				}

			public:

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

				template<typename ObjectType>
				void AddOrUpdatePrimitive(ECS::EntityID EntityID, ObjectType&& Primitive)
				{
					std::get<PackedVector<ObjectType>>(m_ObjectTuple).AddOrUpdate(EntityID, std::forward<ObjectType>(Primitive));
				}

				template<typename ObjectType>
				void RemovePrimitive(ECS::EntityID EntityID)
				{
					std::get<PackedVector<ObjectType>>(m_ObjectTuple).Remove(EntityID);
				}

				template<typename ObjectType>
				const PackedVector<ObjectType>& GetObjects() const
				{
					return std::get<PackedVector<ObjectType>>(m_ObjectTuple);
				}

				std::tuple<PackedVector<Args>...>& GetPrimitiveTuple() { return m_ObjectTuple; }
			};

			using RenderProxy = RenderProxyClass<Scene::StaticMesh, DirectionalLightData, PointLightData, SpotLightData, Scene::Camera>;

		private:
			SceneID m_ID;
			ECS::EntityManager m_EntityManager;
			ECS::ComponentManagerDecl m_ComponentManager;

			// Actual SceneEntity instances that's guaranteed the same address during their lifetime
			std::unordered_map<ECS::EntityID, SceneEntity> m_Entities;

			// Map used as an intermediate lookup to guarantee the SceneEntity's address staying the same throughout its lifetime
			std::unordered_map<std::string, ECS::EntityID> m_NameToEntityID;

			// Map used to allow SceneEntity name lookup
			std::unordered_map<ECS::EntityID, std::string> m_EntityIDToName;

			RenderProxy m_RenderProxy;

			void MoveOp(Scene& Other) noexcept
			{
				m_ID = Other.m_ID;
				m_EntityManager = std::move(Other.m_EntityManager);
				m_ComponentManager = std::move(Other.m_ComponentManager);
				m_RenderProxy = std::move(Other.m_RenderProxy);
				m_NameToEntityID = std::move(Other.m_NameToEntityID);
				m_Entities = std::move(Other.m_Entities);
				m_EntityIDToName = std::move(Other.m_EntityIDToName);

				Other.m_ID = InvalidSceneID;
			}

		public:
			Scene(SceneID ID, ID3D12Device* Device)
				:m_ID(ID)
			{
				// TODO: Remove the need to init all new component type
				constexpr uint32_t MaxEntities = 1000;
				m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::DirectionalLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::PointLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::SpotLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::CameraComponent>().Init(MaxEntities);
				//
			}

			~Scene()
			{
				if (m_ID != InvalidSceneID)
				{
					// TODO: Implement faster clearing if needed
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
				if(m_NameToEntityID.count(Name) > 0)
				{
					std::string Temp = NewName + "_" + std::to_string(AddNum);
					while(m_NameToEntityID.count(Temp) > 0)
					{
						Temp = NewName + "_" + std::to_string(AddNum);
						AddNum++;
					}
					NewName = Temp;
				}

				const ECS::Entity Entity = m_EntityManager.CreateEntity();
				m_EntityIDToName[Entity.GetID()] = NewName;
				m_NameToEntityID[NewName] = Entity.GetID();
				return &(m_Entities[Entity.GetID()] = std::move(SceneEntity(Entity, m_ID)));
			}


			void RemoveEntity(SceneEntity& Entity)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return;
				}

				ECS::EntityID ID = Entity.GetEntityID();
				std::apply([ID](auto&& ... args)
					{
						(args.Remove(ID), ...);
					}, m_RenderProxy.GetPrimitiveTuple()
						);

				std::apply([&Entity](auto&& ... args)
					{
						(args.RemoveComponent(Entity.m_Entity), ...);
					}, m_ComponentManager.GetComponentTuple()
						);

				m_Entities.erase(ID);
				m_NameToEntityID.erase(m_EntityIDToName.at(ID));
				m_EntityIDToName.erase(ID);
			}

			void RemoveEntity(const std::string& Name)
			{
				if (auto Entity = m_NameToEntityID.find(Name); Entity != m_NameToEntityID.end())
				{
					this->RemoveEntity(m_Entities.at(Entity->second));
				}
			}

			SceneEntity* GetEntity(const std::string& Name)
			{
				if (auto Entity = m_NameToEntityID.find(Name); Entity != m_NameToEntityID.end())
				{
					return &m_Entities.at(Entity->second);
				}

				return nullptr;
			}

			void ChangeEntityName(SceneEntity& Entity, const std::string& NewName)
			{
				if (auto ExistingWithName = m_NameToEntityID.find(NewName); ExistingWithName != m_NameToEntityID.end())
				{
					return;
				}

				const ECS::EntityID ID = Entity.GetEntityID();

				m_NameToEntityID[NewName] = ID;
				m_NameToEntityID.erase(m_EntityIDToName.at(ID));

				m_EntityIDToName.at(ID) = NewName;
			}

			void ChangeEntityName(const std::string& OldName, const std::string& NewName)
			{
				if (auto Entity = m_NameToEntityID.find(OldName); Entity != m_NameToEntityID.end())
				{
					this->ChangeEntityName(m_Entities.at(Entity->second), NewName);
				}
			}

			template<typename Component>
			void AddComponent(SceneEntity& Entity, Component&& InComponent)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return;
				}

				m_ComponentManager.AddComponent<Component>(Entity.m_Entity, std::forward<Component>(InComponent));
			}

			template<typename Component>
			void RemoveComponent(SceneEntity& Entity)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return;
				}

				// Fails silently if no component exists
				m_ComponentManager.RemoveComponent<Component>(Entity.m_Entity);
			}

			template<typename Component>
			Component* GetComponent(const SceneEntity& Entity)
			{
				if (Entity.GetSceneID() != m_ID)
				{
					return nullptr;
				}

				// Fails silently and returns nullptr if no component exists
				return m_ComponentManager.GetComponent<Component>(Entity.m_Entity);
			}

			template<typename ObjectType>
			const std::span<const ObjectType> GetObjects() const
			{
				return m_RenderProxy.GetObjects<ObjectType>().GetData();
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

				ECS::TransformComponent* TransformComp = m_ComponentManager.GetComponent<ECS::TransformComponent>(Entity.m_Entity);
				ECS::StaticMeshComponent* StaticMeshComp = m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(Entity.m_Entity);

				// NOTE: When we have SkeletalMeshComp we will make this if-statement be one or the other
				if (TransformComp && StaticMeshComp)
				{
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
						const DXM::Vector3 Position(WorldMatrix.m[3][0], WorldMatrix.m[3][1], WorldMatrix.m[3][2]);
						MeshPrim.DXBounds = DirectX::BoundingSphere(Position, StaticMeshComp->m_MeshReference->GetData().BoundingSphereRadius * MaxScaleAxis);
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

				ECS::DirectionalLightComponent* DirLightComp = m_ComponentManager.GetComponent<ECS::DirectionalLightComponent>(Entity.m_Entity);
				if (DirLightComp)
				{
					DirectionalLightData DLData = DirLightComp->GetData();
					m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(DLData));
				}

				ECS::PointLightComponent* PointLightComp = m_ComponentManager.GetComponent<ECS::PointLightComponent>(Entity.m_Entity);
				if (PointLightComp)
				{
					PointLightData PLData = PointLightComp->GetData();

					// TODO: Impl parenting
					if (/*TransformComp*/false)
					{
						PLData.Position = DXM::Vector3::Transform(PLData.Position, TransformComp->GetTransform());
					}

					m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(PLData));
				}

				ECS::SpotLightComponent* SpotLightComp = m_ComponentManager.GetComponent<ECS::SpotLightComponent>(Entity.m_Entity);
				if (SpotLightComp)
				{
					SpotLightData SLData = SpotLightComp->GetData();

					// TODO: Impl parenting
					if (/*TransformComp*/false)
					{
						SLData.Position += {TransformComp->GetTransform().m[3][0], TransformComp->GetTransform().m[3][1], TransformComp->GetTransform().m[3][2]};
						SLData.Direction = DXM::Vector3::TransformNormal(SLData.Direction, TransformComp->GetTransform());
						SLData.Direction.Normalize();
					}

					m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(SLData));
				}

				ECS::CameraComponent* CameraComp = m_ComponentManager.GetComponent<ECS::CameraComponent>(Entity.m_Entity);
				if (CameraComp)
				{
					// TODO: Impl parenting
					Camera CamPrim;
					CamPrim.IsActive = true;
					CamPrim.Viewport = CameraComp->GetViewport();
					CamPrim.ScizzorRect = CameraComp->GetScizzorRect();
					CamPrim.ViewMatrix = CameraComp->GetViewMatrix();
					CamPrim.ProjMatrix = CameraComp->GetProjectionMatrix();
					m_RenderProxy.AddOrUpdatePrimitive(Entity.GetEntityID(), std::move(CamPrim));
				}
			}

			void MarkRenderStateDirty(const std::string& EntityName)
			{
				if (auto Entity = m_NameToEntityID.find(EntityName); Entity != m_NameToEntityID.end())
				{
					this->MarkRenderStateDirty(m_Entities.at(Entity->second));
				}
			}
		};
	}
}