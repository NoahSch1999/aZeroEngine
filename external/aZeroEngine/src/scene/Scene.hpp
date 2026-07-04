#pragma once
#include <bitset>
#include "misc/HelperFunctions.hpp"

#include "LinearAllocator.hpp"
#include "renderer/GPU_Structs.hpp"
#include "render_api/resource/buffer/Buffer.hpp"

namespace aZero
{
	namespace Asset { class AssetManager; }
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

			std::tuple<SceneRenderDataFrameInfo, std::reference_wrapper<SceneRenderData>> GetRenderData(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, aZero::RenderAPI::CommandList& cmdList);

			//void AddDebugDrawArguments(Asset::AssetManager& assetManager, Rendering::WireframeRenderer& wireframeRenderer, bool showColliders, bool showMeshBounds);

			bool HasPhysics() const { return m_PhysicsWorld.get() != nullptr; }
			void ApplyPhysics();
			void UpdatePhysics(bool applyImmediate = true);
			void OptimizePhysics();

			void MarkStaticMeshesDirty() { m_ShouldRebuildStaticMeshes = true; }

			/*void RemoveMeshesWith(Asset::RenderID withID);
			void RemoveMeshesWithMaterial(Asset::RenderID withID);*/

			const std::unordered_map<uint32_t, flecs::entity_t>& GetBodyID_To_EntityID_Map() const { return m_BodyID_To_EntityID; } // This name...

			// This is specialized to define custom behavior on asset erase on a per-asset level
			template<typename AssetType>
			void OnAssetErased(AssetType& asset)
			{

			}

		private:
			void Init();

			void RebuildStaticMeshes(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList);

			void ResolveCollisionEvents();
			void RegisterToPhysics(flecs::entity entity, Component::Rigidbody& rigidbody);
			void UnregisterFromPhysics(flecs::entity entity, Component::Rigidbody& rigidbody);

			flecs::world m_World;
			flecs::observer m_Physics_OnSet_Observer;
			flecs::observer m_Physics_OnRemove_Observer;

			flecs::query<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale> m_Static_Mesh_Query;

			flecs::query<Component::Mesh, Component::Position, Component::Rotation, Component::Scale> m_Dynamic_Mesh_Query;
			flecs::query<Component::Camera, Component::Position, Component::Rotation> m_CameraQuery;
			flecs::query<Component::Rigidbody, Component::Position, Component::Rotation> m_ApplyPhysicsQuery;

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
	}
}