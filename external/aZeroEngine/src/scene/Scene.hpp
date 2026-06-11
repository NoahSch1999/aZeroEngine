#pragma once
#include <bitset>
#include "misc/HelperFunctions.hpp"
#include "misc/SparseSet.hpp"

#include "SceneRenderData.hpp"
#include "LinearAllocator.hpp"

/*
TODO:
fix so shaders use the new structs etc
change so ::Render() uses ::GetRenderData()


*/

namespace aZero
{
	namespace Asset
	{
		class AssetManager;
	}

	namespace Physics
	{
		class PhysicsEngine;
	}

	namespace Rendering
	{
		class WireframeRenderer;
	}

	namespace RenderAPI
	{
		class Buffer;
		class Commandlist;
		class ResourceRecycler;
	}

	class Engine;

	namespace Scene
	{
		struct SceneRenderData
		{
			RenderAPI::Buffer ObjectCullDataBuffer;
			RenderAPI::Buffer InstanceBuffer;
			std::vector<Rendering::GPU_Struct::CameraData> CameraData;
		};

		using SceneID = uint32_t;

		/*  ------------------------------------------------------------
									WARNING!
			The app will crash if the entity is destroyed or a rigidbody component is removed before unregistered using ::UnregisterFromPhysics().
			The design choice is questionable, but this keeps the API clean...
			------------------------------------------------------------
		*/
		class Scene
		{
		public:
			Scene();
			Scene(Physics::PhysicsEngine& physicsEngine);
			~Scene();

			flecs::world& GetEntityWorld() { return m_World; }
			SceneID GetSceneID() const { return m_SceneID; }
			flecs::entity GetStaticMeshPrefab() const { return m_StaticMeshPrefab; }
			flecs::entity GetRigidbodyStaticMeshPrefab() const { return m_RigidbodyStaticMeshPrefab; }
			flecs::entity GetCameraPrefab() const { return m_CameraPrefab; }

			std::tuple<uint32_t, std::reference_wrapper<SceneRenderData>> GetRenderData(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList, bool recache);

			void AddDebugDrawArguments(Asset::AssetManager& assetManager, Rendering::WireframeRenderer& wireframeRenderer, bool showColliders, bool showMeshBounds);

			bool HasPhysics() const { return m_PhysicsWorld.get() != nullptr; }
			void ApplyPhysics();
			void UpdatePhysics(bool applyImmediate = true);
			void OptimizePhysics();
			void RegisterToPhysics(flecs::entity entity);
			void UnregisterFromPhysics(flecs::entity entity);

			void UpdateTemp();

			void RemoveMeshesWith(Asset::RenderID withID);
			void RemoveMeshesWithMaterial(Asset::RenderID withID);

			const std::unordered_map<JPH::BodyID, flecs::entity_t>& GetBodyID_To_EntityID_Map() const { return m_BodyID_To_EntityID; } // This name...
		private:
			void Init();
			void ResolveCollisionEvents();

			flecs::world m_World;

			flecs::query<Component::Mesh, Component::Position, Component::Rotation, Component::Scale> m_MovableMeshQuery;
			flecs::query<Component::Camera, Component::Position, Component::Rotation> m_CameraQuery;
			flecs::query<Component::Rigidbody, Component::Position, Component::Rotation> m_ApplyPhysicsQuery;

			flecs::entity m_StaticMeshPrefab;
			flecs::entity m_RigidbodyStaticMeshPrefab;
			flecs::entity m_CameraPrefab;

			std::unique_ptr<Physics::PhysicsWorld> m_PhysicsWorld;
			std::unordered_map<JPH::BodyID, flecs::entity_t> m_BodyID_To_EntityID;

			SceneID m_SceneID;
			static inline std::atomic<SceneID> m_IncrementingID = 0;

			SceneRenderData m_RenderData;
			uint32_t m_LastCachedEntityIndex;
		};
	}
}