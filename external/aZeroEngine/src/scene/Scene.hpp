#pragma once
#include <bitset>
#include "misc/HelperFunctions.hpp"

#include "LinearAllocator.hpp"
#include "renderer/GPU_Structs.hpp"
#include "render_api/resource/buffer/Buffer.hpp"

namespace aZero
{
	namespace Physics { class PhysicsEngine; }
	namespace Rendering { class WireframeRenderer; }
	namespace RenderAPI { class CommandList; }

	namespace Scene
	{
		struct SceneRenderDataFrameInfo
		{
			uint32_t StaticMeshCount;
		};

		struct SceneRenderData
		{
			// todo Make buffers resizable
			aZero::RenderAPI::Buffer ObjectCullDataBuffer;
			aZero::RenderAPI::Buffer InstanceBuffer;
			aZero::RenderAPI::Buffer CameraBuffer;
			std::vector<D3D12_VIEWPORT> CameraRSData;
		};

		using SceneID = uint32_t;

		class Scene
		{
		public:
			Scene();
			Scene(Physics::PhysicsEngine& physicsEngine);
			~Scene();

			flecs::world& GetEntityWorld() { return m_World; }
			SceneID GetSceneID() const { return m_SceneID; }
			flecs::entity GetStaticMeshPrefab() const { return m_StaticMeshPrefab; }
			flecs::entity GetCameraPrefab() const { return m_CameraPrefab; }
			flecs::query<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale> GetStaticMeshQuery() const { return m_Static_Mesh_Query; }
			flecs::query<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale> GetDynamicMeshQuery() const { return m_Dynamic_Mesh_Query; }
			flecs::query<Component::Rigidbody, Component::Position, Component::Rotation> GetRigidbodyQuery() const { return m_ApplyPhysicsQuery; }
			flecs::query<Component::Triggerbody, Component::Position> GetTriggerbodyQuery() const { return m_TriggerbodyQuery; }

			std::tuple<SceneRenderDataFrameInfo, std::reference_wrapper<SceneRenderData>> GetRenderData(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, aZero::RenderAPI::CommandList& cmdList);

			bool HasPhysics() const { return m_PhysicsWorld.get() != nullptr; }
			void ApplyPhysics();
			void UpdatePhysics(bool applyImmediate = true);
			void OptimizePhysics();

			void MarkStaticMeshesDirty() { m_ShouldRebuildStaticMeshes = true; }

			const std::unordered_map<uint32_t, flecs::entity_t>& GetBodyID_To_EntityID_Map() const { return m_BodyID_To_EntityID; } // This name...

			// This is specialized to define custom behavior on asset erase on a per-asset level
			template<typename AssetType>
			void OnAssetErased(const AssetType& asset);

		private:
			void Init();

			void RebuildStaticMeshes(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList);

			void ResolveCollisionEvents();
			void RegisterToPhysics(flecs::entity entity, Physics::Body& body, JPH::BodyCreationSettings& bodySettings);
			void UnregisterFromPhysics(flecs::entity entity, Physics::Body& body);

			flecs::world m_World;
			flecs::observer m_Rigidbody_OnSet_Observer;
			flecs::observer m_Rigidbody_OnRemove_Observer;
			flecs::observer m_Triggerbody_OnSet_Observer;
			flecs::observer m_Triggerbody_OnRemove_Observer;

			flecs::query<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale> m_Static_Mesh_Query;

			flecs::query<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale> m_Dynamic_Mesh_Query;
			flecs::query<Component::Camera, Component::Position, Component::Rotation> m_CameraQuery;
			flecs::query<Component::Rigidbody, Component::Position, Component::Rotation> m_ApplyPhysicsQuery;
			flecs::query<Component::Triggerbody, Component::Position> m_TriggerbodyQuery;

			flecs::entity m_StaticMeshPrefab;
			flecs::entity m_RigidbodyStaticMeshPrefab;
			flecs::entity m_CameraPrefab;

			std::unique_ptr<Physics::PhysicsWorld> m_PhysicsWorld;
			std::unordered_map<uint32_t, flecs::entity_t> m_BodyID_To_EntityID;

			SceneID m_SceneID;
			static inline std::atomic<SceneID> m_IncrementingID = 0;

			SceneRenderData m_RenderData;
			uint32_t m_NumStaticMeshEntities = 0;
			bool m_ShouldRebuildStaticMeshes = true;
		};

		template<typename AssetType>
		void Scene::OnAssetErased(const AssetType&) { }

		template<>
		inline void Scene::OnAssetErased(const Asset::Mesh& mesh)
		{
			m_World.defer_begin();
			m_World.query<Component::Mesh>().each(
				[mesh](flecs::entity entity, Component::Mesh& meshComponent) {
					if (meshComponent.m_MeshID == mesh.GetRenderRef().m_MeshletGlobalOffset)
					{
						entity.remove<Component::Mesh>();
					}
				}
			);
			m_World.defer_end();
		}

		template<>
		inline void Scene::OnAssetErased(const Asset::Material& material)
		{
			m_World.defer_begin();
			m_World.query<Component::Mesh>().each(
				[material](flecs::entity entity, Component::Mesh& meshComponent) {
					for (auto& submesh : meshComponent.m_Submeshes)
					{
						if (submesh.m_MaterialID == material.GetRenderRef().MaterialIndex)
						{
							// todo Assign the mesh the default material
						}
					}
				}
			);
			m_World.defer_end();
		}
	}
}