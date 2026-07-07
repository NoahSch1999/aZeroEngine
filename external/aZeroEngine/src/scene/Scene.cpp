#include "Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "renderer/WireframeRenderer.hpp"
#include "renderer/GPU_Structs.hpp"
#include "misc/HelperFunctions.hpp"

// todo Test so that submeshes work as expected
void WriteRenderFormat(aZero::Rendering::GPU_Struct::ObjectCullData* pObjCull, aZero::Rendering::GPU_Struct::InstanceData* pInstance,
	const aZero::Component::Mesh& mesh, const aZero::Component::Position& position, const aZero::Component::Rotation& rotation, const aZero::Component::Scale& scale, uint32_t& perObjectIndex, uint32_t& instanceDataIndex, uint32_t baseOffsetInstanceIndex)
{
	using namespace aZero::Rendering;
	GPU_Struct::InstanceData instanceData;
	instanceData.Transform = DXM::Matrix::CreateScale(scale) * DXM::Matrix::CreateFromYawPitchRoll(rotation) * DXM::Matrix::CreateTranslation(position);
	*(pInstance + instanceDataIndex) = instanceData;

	for (uint32_t j = 0; j < mesh.m_NumSubmeshes; j++)
	{
		GPU_Struct::ObjectCullData objectCullData;
		mesh.m_Submeshes[j].m_Bounds.Transform(objectCullData.Bounds, instanceData.Transform);
		objectCullData.GlobalMeshletOffset = mesh.m_Submeshes[j].MeshletGlobalOffset;
		objectCullData.GlobalVertexOffset = mesh.m_Submeshes[j].VertexGlobalOffset;
		objectCullData.MaterialIndex = mesh.m_Submeshes[j].m_MaterialID;
		objectCullData.MeshletCount = mesh.m_Submeshes[j].MeshletCount;
		objectCullData.InstanceDataIndex = instanceDataIndex + baseOffsetInstanceIndex;

		*(pObjCull + perObjectIndex) = objectCullData;

		perObjectIndex++;
	}

	instanceDataIndex++;
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
	m_TriggerbodyQuery = {};

	if (m_PhysicsWorld.get())
	{
		m_Rigidbody_OnSet_Observer.destruct();
		m_Rigidbody_OnRemove_Observer.destruct();
		m_Triggerbody_OnSet_Observer.destruct();
		m_Triggerbody_OnRemove_Observer.destruct();
		m_World.release(); // Need to be destroyed before the physics world since there's observers that use the physics world (unless they are disabled beforehand)
		m_PhysicsWorld.reset();
	}
}

void aZero::Scene::Scene::Init()
{
	m_SceneID = m_IncrementingID.fetch_add(1, std::memory_order_relaxed);

	// Enable remote flecs explorer
	//m_World.set<flecs::Rest>({});

	m_World.component<Component::Position>();
	m_World.component<Component::Rotation>();
	m_World.component<Component::Scale>();
	m_World.component<Component::Camera>();
	m_World.component<Component::Mesh>();
	m_World.component<Component::PointLight>();
	m_World.component<Component::SpotLight>();
	m_World.component<Component::DirectionalLight>();
	m_World.component<Component::Rigidbody>();
	m_World.component<Component::Triggerbody>();
	m_World.component<Component::Static>();

	m_Static_Mesh_Query = m_World.query_builder<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale>().with<Component::Static>().build();
	m_Dynamic_Mesh_Query = m_World.query_builder<const Component::Mesh, const Component::Position, const Component::Rotation, const Component::Scale>().without<Component::Static>().cached().build();
	m_CameraQuery = m_World.query_builder<Component::Camera, Component::Position, Component::Rotation>().cached().build();

	m_StaticMeshPrefab = m_World.prefab("StaticMeshPrefab").set(Component::Mesh()).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0)).set(Component::Scale(1, 1, 1));
	m_CameraPrefab = m_World.prefab("CameraPrefab").set(Component::Camera(3.14f / 2.f, 0.001f, 1000.f, true, { 0,0 }, { 1920, 1080 })).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0));

	if (m_PhysicsWorld.get()) 
	{
		m_RigidbodyStaticMeshPrefab = m_World.prefab("RigidbodyStaticMeshPrefab").set(Component::Rigidbody()).set(Component::Mesh()).set(Component::Position(0, 0, 0)).set(Component::Rotation(0, 0, 0)).set(Component::Scale(1, 1, 1));

		m_ApplyPhysicsQuery = m_World.query_builder<Component::Rigidbody, Component::Position, Component::Rotation>().without<Component::Static>().cached().build();
		m_TriggerbodyQuery = m_World.query_builder<Component::Triggerbody, Component::Position>().cached().build();

		m_Rigidbody_OnSet_Observer = m_World.observer<Component::Rigidbody>().event(flecs::OnSet).each(
			[this](flecs::entity entity, Component::Rigidbody& rb) {
				this->RegisterToPhysics(entity, rb.GetBody(), rb.m_BodySettings);
			}
		);

		m_Rigidbody_OnRemove_Observer = m_World.observer<Component::Rigidbody>().event(flecs::OnRemove).each(
			[this](flecs::entity entity, Component::Rigidbody& rb) {
				this->UnregisterFromPhysics(entity, rb.GetBody());
			}
		);

		m_Triggerbody_OnSet_Observer = m_World.observer<Component::Triggerbody>().event(flecs::OnSet).each(
			[this](flecs::entity entity, Component::Triggerbody& tb) {
				this->RegisterToPhysics(entity, tb.GetBody(), tb.m_BodySettings);
			}
		);

		m_Triggerbody_OnRemove_Observer = m_World.observer<Component::Triggerbody>().event(flecs::OnRemove).each(
			[this](flecs::entity entity, Component::Triggerbody& tb) {
				this->UnregisterFromPhysics(entity, tb.GetBody());
			}
		);
	}
}

void aZero::Scene::Scene::RebuildStaticMeshes(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList)
{
	// todo Reset stuff, ex. tree, for the new cached data
	// todo Rehash mesh in tree etc

	m_Static_Mesh_Query.run([this, &frameDataAllocator, &frameDataBuffer, &cmdList] (flecs::iter& it) {
		using namespace Rendering;
		GPU_Struct::ObjectCullData* pObjCull = reinterpret_cast<GPU_Struct::ObjectCullData*>(frameDataAllocator.Allocate(m_Static_Mesh_Query.count() * Component::Mesh::s_MaxNumberOfSubmeshes * sizeof(GPU_Struct::ObjectCullData)));
		size_t objectCullDataOffset = frameDataAllocator.GetOffset() - m_Static_Mesh_Query.count() * Component::Mesh::s_MaxNumberOfSubmeshes * sizeof(GPU_Struct::ObjectCullData);

		GPU_Struct::InstanceData* pInstance = reinterpret_cast<GPU_Struct::InstanceData*>(frameDataAllocator.Allocate(m_Static_Mesh_Query.count() * Component::Mesh::s_MaxNumberOfSubmeshes * sizeof(GPU_Struct::InstanceData)));
		size_t instanceDataOffset = frameDataAllocator.GetOffset() - m_Static_Mesh_Query.count() * Component::Mesh::s_MaxNumberOfSubmeshes * sizeof(GPU_Struct::InstanceData);

		uint32_t numUniqueMeshes = 0;
		uint32_t numUniqueInstanceData = 0;

		while (it.next())
		{
			auto mesh = it.field<const Component::Mesh>(0);
			auto position = it.field<const Component::Position>(1);
			auto rotation = it.field<const Component::Rotation>(2);
			auto scale = it.field<const Component::Scale>(3);

			for (auto i : it)
			{
				WriteRenderFormat(pObjCull, pInstance, mesh[i], position[i], rotation[i], scale[i], numUniqueMeshes, numUniqueInstanceData, 0);
			}
		}

		m_NumUniqueStaticMeshes = numUniqueMeshes;
		m_NumUniqueStaticInstanceData = numUniqueInstanceData;

		cmdList->CopyBufferRegion(m_RenderData.ObjectCullDataBuffer.GetResource(), 0, frameDataBuffer.GetResource(), objectCullDataOffset, m_NumUniqueStaticMeshes * sizeof(GPU_Struct::ObjectCullData));
		cmdList->CopyBufferRegion(m_RenderData.InstanceBuffer.GetResource(), 0, frameDataBuffer.GetResource(), instanceDataOffset, m_NumUniqueStaticInstanceData * sizeof(GPU_Struct::InstanceData));
	});
}

uint32_t aZero::Scene::Scene::UploadDynamicMeshes(aZero::LinearAllocator<>& frameDataAllocator, RenderAPI::Buffer& frameDataBuffer, RenderAPI::CommandList& cmdList)
{
	using namespace Rendering;
	uint32_t entityUpdateCount = m_Dynamic_Mesh_Query.count() * Component::Mesh::s_MaxNumberOfSubmeshes;

	GPU_Struct::ObjectCullData* pObjCull = reinterpret_cast<GPU_Struct::ObjectCullData*>(frameDataAllocator.Allocate(entityUpdateCount * sizeof(GPU_Struct::ObjectCullData)));
	size_t objectCullDataOffset = frameDataAllocator.GetOffset() - entityUpdateCount * sizeof(GPU_Struct::ObjectCullData);

	GPU_Struct::InstanceData* pInstance = reinterpret_cast<GPU_Struct::InstanceData*>(frameDataAllocator.Allocate(entityUpdateCount * sizeof(GPU_Struct::InstanceData)));
	size_t instanceDataOffset = frameDataAllocator.GetOffset() - entityUpdateCount * sizeof(GPU_Struct::InstanceData);

	uint32_t numUniqueMeshes = 0;
	uint32_t numUniqueInstanceData = 0;
	m_Dynamic_Mesh_Query.each([pObjCull, pInstance, &numUniqueMeshes, &numUniqueInstanceData, this](const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
		{
			WriteRenderFormat(pObjCull, pInstance, mesh, position, rotation, scale, numUniqueMeshes, numUniqueInstanceData, m_NumUniqueStaticInstanceData);
		});

	cmdList->CopyBufferRegion(m_RenderData.ObjectCullDataBuffer.GetResource(),
		m_NumUniqueStaticMeshes * sizeof(GPU_Struct::ObjectCullData), // Copy from the end of the cached mesh entities
		frameDataBuffer.GetResource(), objectCullDataOffset, numUniqueMeshes * sizeof(GPU_Struct::ObjectCullData));

	cmdList->CopyBufferRegion(m_RenderData.InstanceBuffer.GetResource(),
		m_NumUniqueStaticInstanceData * sizeof(GPU_Struct::InstanceData), // Copy from the end of the cached mesh entities
		frameDataBuffer.GetResource(), instanceDataOffset, numUniqueInstanceData * sizeof(GPU_Struct::InstanceData));

	return m_NumUniqueStaticMeshes + numUniqueMeshes;
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

	return { SceneRenderDataFrameInfo{.MeshCount = this->UploadDynamicMeshes(frameDataAllocator, frameDataBuffer, cmdList) }, m_RenderData };
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

void aZero::Scene::Scene::RegisterToPhysics(flecs::entity entity, Physics::Body& body, JPH::BodyCreationSettings& bodySettings)
{
	if (m_BodyID_To_EntityID.count(body.GetBodyID().GetIndexAndSequenceNumber()))
	{
		this->UnregisterFromPhysics(entity, body);
	}

	bodySettings.mUserData = static_cast<uint64_t>(entity.id());
	body = m_PhysicsWorld->CreateBody(bodySettings, true);
	m_BodyID_To_EntityID[body.GetBodyID().GetIndexAndSequenceNumber()] = entity.id();
}

void aZero::Scene::Scene::UnregisterFromPhysics(flecs::entity entity, Physics::Body& body)
{
	if (m_BodyID_To_EntityID.count(body.GetBodyID().GetIndexAndSequenceNumber()))
	{
		m_BodyID_To_EntityID.erase(body.GetBodyID().GetIndexAndSequenceNumber());
		m_PhysicsWorld->DestroyBody(body);
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