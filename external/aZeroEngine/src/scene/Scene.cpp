#include "Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "renderer/WireframeRenderer.hpp"
#include "assets/AssetManager.hpp"
#include "renderer/GPU_Structs.hpp"

// todo Test so that submeshes work as expected
void WriteRenderFormat(aZero::Rendering::GPU_Struct::ObjectCullData* pObjCull, aZero::Rendering::GPU_Struct::InstanceData* pInstance,
	const aZero::Component::Mesh& mesh, const aZero::Component::Position& position, const aZero::Component::Rotation& rotation, const aZero::Component::Scale& scale, uint32_t& count)
{
	using namespace aZero::Rendering;
	for (uint32_t j = 0; j < mesh.m_NumSubmeshes; j++)
	{
		GPU_Struct::InstanceData instanceData;
		instanceData.Transform = DXM::Matrix::CreateScale(scale) * DXM::Matrix::CreateFromYawPitchRoll(rotation) * DXM::Matrix::CreateTranslation(position);

		GPU_Struct::ObjectCullData objectCullData;
		mesh.m_Submeshes[j].m_Bounds.Transform(objectCullData.Bounds, instanceData.Transform);
		objectCullData.GlobalMeshletOffset = mesh.m_Submeshes[j].MeshletGlobalOffset;
		objectCullData.GlobalVertexOffset = mesh.m_Submeshes[j].VertexGlobalOffset;
		objectCullData.MaterialIndex = mesh.m_Submeshes[j].m_MaterialID;
		objectCullData.MeshletCount = mesh.m_Submeshes[j].MeshletCount;

		*(pObjCull + count) = objectCullData;
		*(pInstance + count) = instanceData;

		count++;
	}
}

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
	m_Dynamic_Mesh_Query = {};
	m_Static_Mesh_Query = {};
	m_CameraQuery = {};
	m_ApplyPhysicsQuery = {};

	if (m_PhysicsWorld.get())
	{
		m_Physics_OnSet_Observer.destruct();
		m_Physics_OnRemove_Observer.destruct();
		m_World.release(); // Need to be destroyed before the physics world since there's observers that use the physics world (unless they are disabled beforehand)
		m_PhysicsWorld.reset();
	}
}

void aZero::Scene::Scene::Init()
{
	m_SceneID = m_IncrementingID.fetch_add(1, std::memory_order_relaxed);

	m_World.component<Component::Position>();
	m_World.component<Component::Rotation>();
	m_World.component<Component::Scale>();
	m_World.component<Component::Camera>();
	m_World.component<Component::Mesh>();
	m_World.component<Component::PointLight>();
	m_World.component<Component::SpotLight>();
	m_World.component<Component::DirectionalLight>();
	m_World.component<Component::Rigidbody>();
	m_World.component<Component::Static>();

	m_Static_Mesh_Query = m_World.query_builder<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale>().with<Component::Static>().build();
	m_Dynamic_Mesh_Query = m_World.query_builder<Component::Mesh, Component::Position, Component::Rotation, Component::Scale>().without<Component::Static>().cached().build();
	m_CameraQuery = m_World.query_builder<Component::Camera, Component::Position, Component::Rotation>().cached().build();

	m_StaticMeshPrefab = m_World.prefab("StaticMeshPrefab").set(Component::Mesh()).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0)).set(Component::Scale(1, 1, 1));
	m_CameraPrefab = m_World.prefab("CameraPrefab").set(Component::Camera(3.14f / 2.f, 0.001f, 1000.f, true, { 0,0 }, { 1920, 1080 })).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0));

	if (m_PhysicsWorld.get()) 
	{
		m_RigidbodyStaticMeshPrefab = m_World.prefab("RigidbodyStaticMeshPrefab").set(Component::Rigidbody()).set(Component::Mesh()).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0)).set(Component::Scale(1, 1, 1));

		m_ApplyPhysicsQuery = m_World.query_builder<Component::Rigidbody, Component::Position, Component::Rotation>().without<Component::Static>().cached().build();

		m_Physics_OnSet_Observer = m_World.observer<Component::Rigidbody>().event(flecs::OnSet).each(
			[this](flecs::entity entity, Component::Rigidbody& rb) {
				this->RegisterToPhysics(entity, rb);
			}
		);

		m_Physics_OnRemove_Observer = m_World.observer<Component::Rigidbody>().event(flecs::OnRemove).each(
			[this](flecs::entity entity, Component::Rigidbody& rb) {
				this->UnregisterFromPhysics(entity, rb);
			}
		);
	}
	
}

void aZero::Scene::Scene::RebuildStaticMeshes(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList)
{
	// todo Reset stuff, ex. tree, for the new cached data
	// todo Rehash mesh in tree etc
	m_NumStaticMeshEntities = m_Static_Mesh_Query.count();
	m_Static_Mesh_Query.run([this, &frameDataAllocator, &frameDataBuffer, &cmdList] (flecs::iter& it) {
		using namespace Rendering;
		size_t objectCullDataOffset = frameDataAllocator.GetOffset();
		GPU_Struct::ObjectCullData* pObjCull = reinterpret_cast<GPU_Struct::ObjectCullData*>(frameDataAllocator.Allocate(this->m_NumStaticMeshEntities * sizeof(GPU_Struct::ObjectCullData)));

		size_t instanceDataOffset = frameDataAllocator.GetOffset();
		GPU_Struct::InstanceData* pInstance = reinterpret_cast<GPU_Struct::InstanceData*>(frameDataAllocator.Allocate(this->m_NumStaticMeshEntities * sizeof(GPU_Struct::InstanceData)));

		uint32_t totalNumEntities = 0;

		while (it.next())
		{
			auto mesh = it.field<const Component::Mesh>(0);
			auto position = it.field<const Component::Position>(1);
			auto rotation = it.field<const Component::Rotation>(2);
			auto scale = it.field<const Component::Scale>(3);

			for (auto i : it)
			{
				WriteRenderFormat(pObjCull, pInstance, mesh[i], position[i], rotation[i], scale[i], totalNumEntities);
			}
		}

		cmdList->CopyBufferRegion(m_RenderData.ObjectCullDataBuffer.GetResource(), 0, frameDataBuffer.GetResource(), objectCullDataOffset, this->m_NumStaticMeshEntities * sizeof(GPU_Struct::ObjectCullData));
		cmdList->CopyBufferRegion(m_RenderData.InstanceBuffer.GetResource(), 0, frameDataBuffer.GetResource(), instanceDataOffset, this->m_NumStaticMeshEntities * sizeof(GPU_Struct::InstanceData));
	});
}

std::tuple<aZero::Scene::SceneRenderDataFrameInfo, std::reference_wrapper<aZero::Scene::SceneRenderData>> aZero::Scene::Scene::GetRenderData(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList)
{
	using namespace Rendering;

	if (!m_RenderData.ObjectCullDataBuffer.GetResource())
	{
		ID3D12DeviceX* device = GetID3D12DeviceX(cmdList.Get());
		m_RenderData.ObjectCullDataBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::ObjectCullData) * 100000, D3D12_HEAP_TYPE_DEFAULT), &frameDataBuffer.GetResourceRecycler());
		m_RenderData.InstanceBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::InstanceData) * 100000, D3D12_HEAP_TYPE_DEFAULT), &frameDataBuffer.GetResourceRecycler());
		m_RenderData.CameraBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::CameraData) * 1000, D3D12_HEAP_TYPE_DEFAULT), &frameDataBuffer.GetResourceRecycler());
	}

	this->m_RenderData.CameraRSData.clear();

	std::vector<Rendering::GPU_Struct::CameraData> cameraGPUData;
	m_CameraQuery.each([this, &cameraGPUData](const Component::Camera& camera, const Component::Position& position, const Component::Rotation& rotation)
		{
			if (camera.isActive)
			{
				GPU_Struct::CameraData cameraData;
				cameraData.ViewMatrix = camera.GetViewMatrix(position, rotation);
				cameraData.ViewProjectionMatrix = camera.GetViewProjectionMatrix(position, rotation);
				cameraData.Frustum = camera.GetFrustum();
				cameraGPUData.emplace_back(cameraData);
				this->m_RenderData.CameraRSData.emplace_back(camera.GetViewport());
			}
		});

	size_t cameraDataOffset = frameDataAllocator.GetOffset();
	GPU_Struct::CameraData* pCameraData = reinterpret_cast<GPU_Struct::CameraData*>(frameDataAllocator.Allocate(cameraGPUData.size() * sizeof(GPU_Struct::CameraData)));
	memcpy(pCameraData, cameraGPUData.data(), cameraGPUData.size() * sizeof(GPU_Struct::CameraData));

	cmdList->CopyBufferRegion(m_RenderData.CameraBuffer.GetResource(),
		0, frameDataBuffer.GetResource(), cameraDataOffset, cameraGPUData.size() * sizeof(GPU_Struct::CameraData));

	if (m_ShouldRebuildStaticMeshes)
	{
		this->RebuildStaticMeshes(frameDataAllocator, frameDataBuffer, cmdList);
		m_ShouldRebuildStaticMeshes = false;
	}

	uint32_t entityUpdateCount = m_Dynamic_Mesh_Query.count();

	size_t objectCullDataOffset = frameDataAllocator.GetOffset();
	GPU_Struct::ObjectCullData* pObjCull = reinterpret_cast<GPU_Struct::ObjectCullData*>(frameDataAllocator.Allocate(entityUpdateCount * sizeof(GPU_Struct::ObjectCullData)));

	size_t instanceDataOffset = frameDataAllocator.GetOffset();
	GPU_Struct::InstanceData* pInstance = reinterpret_cast<GPU_Struct::InstanceData*>(frameDataAllocator.Allocate(entityUpdateCount * sizeof(GPU_Struct::InstanceData)));

	uint32_t numDynamicMeshEntities = 0; // TODO: Set to number of static entities since we dont wanna overwrite the cached ones
	m_Dynamic_Mesh_Query.each([pObjCull, pInstance, &numDynamicMeshEntities](const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
		{
			WriteRenderFormat(pObjCull, pInstance, mesh, position, rotation, scale, numDynamicMeshEntities);
		});

	cmdList->CopyBufferRegion(m_RenderData.ObjectCullDataBuffer.GetResource(),
		m_NumStaticMeshEntities * sizeof(GPU_Struct::ObjectCullData), // Copy from the end of the cached mesh entities
		frameDataBuffer.GetResource(), objectCullDataOffset, entityUpdateCount * sizeof(GPU_Struct::ObjectCullData));

	cmdList->CopyBufferRegion(m_RenderData.InstanceBuffer.GetResource(),
		m_NumStaticMeshEntities * sizeof(GPU_Struct::InstanceData), // Copy from the end of the cached mesh entities
		frameDataBuffer.GetResource(), instanceDataOffset, entityUpdateCount * sizeof(GPU_Struct::InstanceData));

	return { SceneRenderDataFrameInfo{.StaticMeshCount = numDynamicMeshEntities + m_NumStaticMeshEntities }, m_RenderData };
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
				auto lock = rigidBody.m_Body.LockForRead();
				if (lock.Succeeded())
				{
					auto& body = lock.GetBody();

					auto bounds = body.GetWorldSpaceBounds();

					auto* shape = body.GetShape();
					if (body.GetShape()->GetSubType() == JPH::EShapeSubType::Box)
					{
						const JPH::BoxShape* boxShape = static_cast<const JPH::BoxShape*>(body.GetShape());
						wireframeRenderer.AddShape(Rendering::WireframeShape::OBB(DXM::Vector3(0, 1, 0), Math::Convert(body.GetPosition()), Math::Convert(body.GetRotation()), Math::Convert(boxShape->GetHalfExtent())));
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
			auto lock = rigidBody.m_Body.LockForRead();
			if (lock.Succeeded())
			{
				auto& body = lock.GetBody();
				position = Math::Convert(body.GetPosition());
				rotation = Math::Convert(body.GetRotation()).ToEuler();
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

void aZero::Scene::Scene::RegisterToPhysics(flecs::entity entity, Component::Rigidbody& rigidbody)
{
	if (m_BodyID_To_EntityID.count(rigidbody.m_Body.GetBodyID().GetIndexAndSequenceNumber()))
	{
		this->UnregisterFromPhysics(entity, rigidbody);
	}

	rigidbody.m_BodySettings.mUserData = static_cast<uint64_t>(entity.id());
	rigidbody.m_Body = m_PhysicsWorld->CreateBody(rigidbody.m_BodySettings, true);
	m_BodyID_To_EntityID[rigidbody.m_Body.GetBodyID().GetIndexAndSequenceNumber()] = entity.id();
}

void aZero::Scene::Scene::UnregisterFromPhysics(flecs::entity entity, Component::Rigidbody& rigidbody)
{
	if (m_BodyID_To_EntityID.count(rigidbody.m_Body.GetBodyID().GetIndexAndSequenceNumber()))
	{
		m_BodyID_To_EntityID.erase(rigidbody.m_Body.GetBodyID().GetIndexAndSequenceNumber());
		m_PhysicsWorld->DestroyBody(rigidbody.m_Body);
	}
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
						firstRb.m_OnContactValidate.value()(secondRb.m_Body, event.InBaseOffset, event.InCollisionResult);
					}

					if (secondRb.m_OnContactValidate.has_value()) {
						secondRb.m_OnContactValidate.value()(firstRb.m_Body, event.InBaseOffset, event.InCollisionResult);
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
			if (m_BodyID_To_EntityID.count(event.InSubShapePair.GetBody1ID().GetIndexAndSequenceNumber()) && m_BodyID_To_EntityID.count(event.InSubShapePair.GetBody2ID().GetIndexAndSequenceNumber()))
			{
				flecs::entity firstEntity = m_World.entity(m_BodyID_To_EntityID.at(event.InSubShapePair.GetBody1ID().GetIndexAndSequenceNumber()));
				flecs::entity secondEntity = m_World.entity(m_BodyID_To_EntityID.at(event.InSubShapePair.GetBody2ID().GetIndexAndSequenceNumber()));

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
