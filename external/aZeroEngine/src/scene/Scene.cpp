#include "Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "renderer/WireframeRenderer.hpp"
#include "assets/AssetManager.hpp"
#include "renderer/GPU_Driven_Pipeline_Structs.hpp"

aZero::Scene::Scene::Scene()
{
	this->Init();
}

aZero::Scene::Scene::Scene(Physics::PhysicsEngine& physicsEngine)
	:m_PhysicsWorld(std::make_unique<Physics::PhysicsWorld>())
{
	physicsEngine.CreateWorld(*m_PhysicsWorld.get());
	this->Init();
}

aZero::Scene::Scene::~Scene()
{
	// Reset queries since they should live shorter than the world
	m_MovableMeshQuery = {};
	m_CameraQuery = {};
	m_ApplyPhysicsQuery = {};
}

void aZero::Scene::Scene::Init()
{
	m_SceneID = m_IncrementingID.fetch_add(1, std::memory_order_relaxed);

	// TODO: Make prefab use default mesh and material
	m_StaticMeshPrefab = m_World.prefab("StaticMeshPrefab").set(Component::Mesh()).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0)).set(Component::Scale(1, 1, 1));
	m_RigidbodyStaticMeshPrefab = m_World.prefab("RigidbodyStaticMeshPrefab").set(Component::Rigidbody()).set(Component::Mesh()).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0)).set(Component::Scale(1, 1, 1));
	m_CameraPrefab = m_World.prefab("CameraPrefab").set(Component::Camera(3.14f / 2.f, 0.001f, 1000.f, true, { 0,0 }, { 1920, 1080 })).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0));

	m_MovableMeshQuery = m_World.query_builder<Component::Mesh, Component::Position, Component::Rotation, Component::Scale>().cached().build(); // TODO: Check if it has movable component
	m_CameraQuery = m_World.query_builder<Component::Camera, Component::Position, Component::Rotation>().cached().build();
	m_ApplyPhysicsQuery = m_World.query_builder<Component::Rigidbody, Component::Position, Component::Rotation>().cached().build();
}

void aZero::Scene::Scene::UpdateTemp()
{/*
	auto tempQuery = m_World.query_builder<Component::Mesh, Component::Position, Component::Rotation, Component::Scale>().without<Component::Rigidbody>().cached().build();
	tempQuery.each([](Component::Mesh& mesh, Component::Position& position, Component::Rotation& rotation, Component::Scale& scale)
		{
			rotation.y += 3.14/100.f;
		});*/
}

std::tuple<uint32_t, std::reference_wrapper<aZero::Scene::SceneRenderData>> aZero::Scene::Scene::GetRenderData(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList, bool recache)
{
	using namespace Rendering;

	bool shouldRecache = recache;
	if (!m_RenderData.ObjectCullDataBuffer.GetResource())
	{
		ID3D12DeviceX* device = GetID3D12DeviceX(cmdList.Get());
		m_RenderData.ObjectCullDataBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::ObjectCullData) * 100000, D3D12_HEAP_TYPE_DEFAULT), &frameDataBuffer.GetResourceRecycler());
		m_RenderData.InstanceBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::InstanceData) * 100000, D3D12_HEAP_TYPE_DEFAULT), &frameDataBuffer.GetResourceRecycler());
		shouldRecache = true; // Force a rebuild of the cache if its the first time
	}

	m_CameraQuery.each([this](const Component::Camera& camera, const Component::Position& position, const Component::Rotation& rotation)
		{
			if (camera.isActive)
			{
				GPU_Struct::CameraData cameraData;
				cameraData.ViewMatrix = camera.GetViewMatrix(position, rotation);
				cameraData.ViewProjectionMatrix = cameraData.ViewMatrix * camera.GetProjectionMatrix();
				cameraData.Frustum = camera.GetFrustum();
				this->m_RenderData.CameraData.emplace_back(cameraData);
			}
		});

	uint32_t entityUpdateCount = shouldRecache ? m_MovableMeshQuery.count() /* TODO: + number of stationary ones */ : m_MovableMeshQuery.count() - m_LastCachedEntityIndex;

	// TODO: Handle resize of renderdata buffers
	size_t objectCullDataOffset = frameDataAllocator.GetOffset();
	GPU_Struct::ObjectCullData* pObjCull = static_cast<GPU_Struct::ObjectCullData*>(frameDataAllocator.Allocate(entityUpdateCount * sizeof(GPU_Struct::ObjectCullData)));

	size_t instanceDataOffset = frameDataAllocator.GetOffset();
	GPU_Struct::InstanceData* pInstance = static_cast<GPU_Struct::InstanceData*>(frameDataAllocator.Allocate(entityUpdateCount * sizeof(GPU_Struct::InstanceData)));

	if (shouldRecache)
	{
		m_LastCachedEntityIndex = 0;

		// TODO: Append static meshes to "objectCullDataOffset" etc

	}

	uint32_t totalNumEntities = 0; // TODO: Set to number of static entities since we dont wanna overwrite the cached ones
	m_MovableMeshQuery.each([pObjCull, pInstance, &totalNumEntities](const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
		{
			for (uint32_t i = 0; i < mesh.m_NumSubmeshes; i++)
			{
				GPU_Struct::InstanceData instanceData;
				instanceData.Transform = DXM::Matrix::CreateScale(scale) * DXM::Matrix::CreateFromYawPitchRoll(rotation) * DXM::Matrix::CreateTranslation(position);

				GPU_Struct::ObjectCullData objectCullData;
				mesh.m_Submeshes[i].m_Bounds.Transform(objectCullData.Bounds, instanceData.Transform);
				objectCullData.GlobalMeshletOffset = mesh.m_Submeshes[i].MeshletGlobalOffset;
				objectCullData.GlobalVertexOffset = mesh.m_Submeshes[i].VertexGlobalOffset;
				objectCullData.MaterialIndex = mesh.m_Submeshes[i].m_MaterialID;

				*(pObjCull + totalNumEntities) = objectCullData;
				*(pInstance + totalNumEntities) = instanceData;

				totalNumEntities++;
			}
		});

	size_t copyDstOffsetIndex = shouldRecache ? 0 : m_LastCachedEntityIndex; // If recached => copy from 0, else => copy from the end of the cached entities

	cmdList->CopyBufferRegion(m_RenderData.ObjectCullDataBuffer.GetResource(),
		copyDstOffsetIndex * sizeof(GPU_Struct::ObjectCullData),
		frameDataBuffer.GetResource(), objectCullDataOffset, entityUpdateCount * sizeof(GPU_Struct::ObjectCullData));

	cmdList->CopyBufferRegion(m_RenderData.InstanceBuffer.GetResource(),
		copyDstOffsetIndex * sizeof(GPU_Struct::InstanceData),
		frameDataBuffer.GetResource(), objectCullDataOffset, entityUpdateCount * sizeof(GPU_Struct::InstanceData));

	return { totalNumEntities, m_RenderData };
}

void aZero::Scene::Scene::RemoveMeshesWith(Asset::RenderID withID)
{
	m_World.defer_begin();
	m_World.query<Component::Mesh>().each(
		[withID] (flecs::entity entity, Component::Mesh& mesh) {
			if (mesh.GetMeshID() == withID)
			{
				entity.remove<Component::Mesh>();
			}
		}
	);
	m_World.defer_end();
}

void aZero::Scene::Scene::RemoveMeshesWithMaterial(Asset::RenderID withID)
{
	m_World.defer_begin();
	m_World.query<Component::Mesh>().each(
		[withID](flecs::entity entity, Component::Mesh& mesh) {
			for (uint32_t i = 0; i < mesh.m_NumSubmeshes; i++)
			{
				if (mesh.m_Submeshes[i].m_MaterialID == withID)
				{
					entity.remove<Component::Mesh>();
				}
			}
			
		}
	);
	m_World.defer_end();
}

void aZero::Scene::Scene::AddDebugDrawArguments(Asset::AssetManager& assetManager, Rendering::WireframeRenderer& wireframeRenderer, bool showColliders, bool showMeshBounds)
{
	if (showMeshBounds)
	{
		auto meshQuery = m_World.query<Component::Mesh, Component::Position>();
		const auto& allMeshes = assetManager.GetAllMeshes();
		meshQuery.each([&wireframeRenderer, &allMeshes](const Component::Mesh& mesh, const Component::Position& position) {

			// Lmao this is so bad, but whatever... It's just for debugging... :P
			for (const auto& [name, meshAsset] : allMeshes) 
			{
				if (meshAsset.GetRenderID() == mesh.GetMeshID()) 
				{
					for (const auto& submesh : meshAsset.GetSubmeshes())
					{
						wireframeRenderer.AddShape(Rendering::WireframeShape::Sphere(DXM::Vector3(0, 0, 1), DXM::Vector3::Transform(submesh.Bounds.Center, DXM::Matrix::CreateTranslation(position)), submesh.Bounds.Radius, 10));
					}
				}
			}
		});
	}

	if (showColliders)
	{
		if (m_PhysicsWorld.get())
		{
			m_ApplyPhysicsQuery.each([&wireframeRenderer](Component::Rigidbody& rigidBody, Component::Position& position, Component::Rotation& rotation) {
				auto [lock, body] = rigidBody.m_Body.LockForRead();
				if (lock->Succeeded())
				{
					auto bounds = body->GetWorldSpaceBounds();

					auto* shape = body->GetShape();
					//const JPH::BoxShape* boxShape = dynamic_cast<const JPH::BoxShape*>(body->GetShape()); // Why crash with dynamic cast? Answer: RTTI OFF :(
					if (body->GetShape()->GetSubType() == JPH::EShapeSubType::Box)
					{
						const JPH::BoxShape* boxShape = static_cast<const JPH::BoxShape*>(body->GetShape());
						wireframeRenderer.AddShape(Rendering::WireframeShape::OBB(DXM::Vector3(0, 1, 0), Math::Convert(body->GetPosition()), Math::Convert(body->GetRotation()), Math::Convert(boxShape->GetHalfExtent())));
					}
				}
			});
		}
	}
}

void aZero::Scene::Scene::ApplyPhysics()
{
	if (m_PhysicsWorld.get())
	{
		this->ResolveCollisionEvents();

		m_ApplyPhysicsQuery.each([](Component::Rigidbody& rigidBody, Component::Position& position, Component::Rotation& rotation) {
			auto [lock, body] = rigidBody.m_Body.LockForRead();
			if (lock->Succeeded())
			{
				position = Math::Convert(body->GetPosition());
				rotation = Math::Convert(body->GetRotation()).ToEuler();
			}
		});
	}
}

void aZero::Scene::Scene::UpdatePhysics(bool applyImmediate)
{
	if (m_PhysicsWorld.get())
	{
		m_PhysicsWorld->Update();
		if (applyImmediate) {
			this->ApplyPhysics();
		}
	}
}

void aZero::Scene::Scene::OptimizePhysics()
{
	if (m_PhysicsWorld.get())
	{
		m_PhysicsWorld->OptimizeBroadPhase();
	}
}

void aZero::Scene::Scene::RegisterToPhysics(flecs::entity entity)
{
	if (!m_PhysicsWorld.get())
		return;

	Component::Rigidbody& rb = entity.get_mut<Component::Rigidbody>();
	rb.m_TempBodySettings.mUserData = static_cast<uint64_t>(entity.id());

	rb.m_Body = m_PhysicsWorld->CreateBody(rb.m_TempBodySettings, true);
	m_BodyID_To_EntityID[rb.m_Body.GetBodyID()] = entity.id();
}

void aZero::Scene::Scene::UnregisterFromPhysics(flecs::entity entity)
{
	if (!m_PhysicsWorld.get())
		return;

	Component::Rigidbody& rb = entity.get_mut<Component::Rigidbody>();
	m_BodyID_To_EntityID.erase(rb.m_Body.GetBodyID());
	m_PhysicsWorld->DestroyBody(rb.m_Body);
}

void aZero::Scene::Scene::ResolveCollisionEvents()
{
	if (m_PhysicsWorld.get())
	{
		for (auto& event : m_PhysicsWorld->GetBodyActivatedEvents()) {
			flecs::entity entity = m_World.entity(event.EntityID);
			if (entity.is_alive() && entity.has<Component::Rigidbody>())
			{
				Component::Rigidbody& rb = entity.get_mut<Component::Rigidbody>();
				if (rb.m_OnBodyActivated.has_value()) {
					rb.m_OnBodyActivated.value()();
				}
			}
		}

		for (auto& event : m_PhysicsWorld->GetBodyDeactivatedEvents()) {
			flecs::entity entity = m_World.entity(event.EntityID);
			if (entity.is_alive())
			{
				if (entity.has<Component::Rigidbody>())
				{
					Component::Rigidbody& rb = entity.get_mut<Component::Rigidbody>();
					if (rb.m_OnBodyDeactivated.has_value()) {
						rb.m_OnBodyDeactivated.value()();
					}
				}
			}
		}

		// For both bodies in a collision we call the collision functions with the other body as an argument
		for (auto& event : m_PhysicsWorld->GetContactValidateEvents()) {
			flecs::entity firstEntity = m_World.entity(event.FirstEntityID);
			flecs::entity secondEntity = m_World.entity(event.SecondEntityID);
			if (firstEntity.is_alive() && secondEntity.is_alive())
			{
				if (firstEntity.has<Component::Rigidbody>() && secondEntity.has<Component::Rigidbody>())
				{
					Component::Rigidbody& firstRb = firstEntity.get_mut<Component::Rigidbody>();
					Component::Rigidbody& secondRb = secondEntity.get_mut<Component::Rigidbody>();

					if (firstRb.m_OnContactValidate.has_value()) {
						firstRb.m_OnContactValidate.value()(secondRb.m_Body, event.InBaseOffset, *event.InCollisionResult.get());
					}

					if (secondRb.m_OnContactValidate.has_value()) {
						secondRb.m_OnContactValidate.value()(firstRb.m_Body, event.InBaseOffset, *event.InCollisionResult.get());
					}
				}
			}
		}

		for (auto& event : m_PhysicsWorld->GetContactAddedEvents()) {
			flecs::entity firstEntity = m_World.entity(event.FirstEntityID);
			flecs::entity secondEntity = m_World.entity(event.SecondEntityID);
			if (firstEntity.is_alive() && secondEntity.is_alive())
			{
				if (firstEntity.has<Component::Rigidbody>() && secondEntity.has<Component::Rigidbody>())
				{
					Component::Rigidbody& firstRb = firstEntity.get_mut<Component::Rigidbody>();
					Component::Rigidbody& secondRb = secondEntity.get_mut<Component::Rigidbody>();

					if (firstRb.m_OnContactAdded.has_value()) {
						firstRb.m_OnContactAdded.value()(secondRb.m_Body, event.ContactManifold, event.ContactSettings);
					}

					if (secondRb.m_OnContactAdded.has_value()) {
						secondRb.m_OnContactAdded.value()(firstRb.m_Body, event.ContactManifold, event.ContactSettings);
					}
				}
			}
		}

		for (auto& event : m_PhysicsWorld->GetContactPersistedEvents()) {
			flecs::entity firstEntity = m_World.entity(event.FirstEntityID);
			flecs::entity secondEntity = m_World.entity(event.SecondEntityID);

			if (firstEntity.is_alive() && secondEntity.is_alive())
			{
				if (firstEntity.has<Component::Rigidbody>() && secondEntity.has<Component::Rigidbody>())
				{
					Component::Rigidbody& firstRb = firstEntity.get_mut<Component::Rigidbody>();
					Component::Rigidbody& secondRb = secondEntity.get_mut<Component::Rigidbody>();

					if (firstRb.m_OnContactPersisted.has_value()) {
						firstRb.m_OnContactPersisted.value()(secondRb.m_Body, event.ContactManifold, event.ContactSettings);
					}

					if (secondRb.m_OnContactPersisted.has_value()) {
						secondRb.m_OnContactPersisted.value()(firstRb.m_Body, event.ContactManifold, event.ContactSettings);
					}
				}
			}
		}

		for (auto& event : m_PhysicsWorld->GetContactRemovedEvents()) {
			if (m_BodyID_To_EntityID.count(event.InSubShapePair.GetBody1ID()) && m_BodyID_To_EntityID.count(event.InSubShapePair.GetBody2ID()))
			{
				flecs::entity firstEntity = m_World.entity(m_BodyID_To_EntityID.at(event.InSubShapePair.GetBody1ID()));
				flecs::entity secondEntity = m_World.entity(m_BodyID_To_EntityID.at(event.InSubShapePair.GetBody2ID()));

				if (firstEntity.has<Component::Rigidbody>() && secondEntity.has<Component::Rigidbody>())
				{
					Component::Rigidbody& firstRb = firstEntity.get_mut<Component::Rigidbody>();
					Component::Rigidbody& secondRb = secondEntity.get_mut<Component::Rigidbody>();

					if (firstRb.m_OnContactRemoved.has_value()) {
						firstRb.m_OnContactRemoved.value()(event.InSubShapePair);
					}

					if (secondRb.m_OnContactRemoved.has_value()) {
						secondRb.m_OnContactRemoved.value()(event.InSubShapePair);
					}
				}
			}
		}
		//

		m_PhysicsWorld->ResetBodyEvents();
	}
}
