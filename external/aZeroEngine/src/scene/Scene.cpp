#include "Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "renderer/WireframeRenderer.hpp"
#include "assets/AssetManager.hpp"

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
	m_StaticMeshQuery = {};
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

	m_StaticMeshQuery = m_World.query_builder<Component::Mesh, Component::Position, Component::Rotation, Component::Scale>().cached().build();
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

std::tuple<std::vector<aZero::Rendering::GPUProxy::StaticMesh>, std::vector<aZero::Rendering::GPUProxy::Camera>, std::vector<aZero::Rendering::GPUProxy::Camera::RasterInfo>> aZero::Scene::Scene::GetWorldRenderData() const
{
	std::vector<Rendering::GPUProxy::StaticMesh> meshes;
	meshes.reserve(m_StaticMeshQuery.count());
	m_StaticMeshQuery.each([&meshes](const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
		{
			meshes.emplace_back(mesh, position, rotation, scale);
		});

	std::vector<Rendering::GPUProxy::Camera> cameras;
	std::vector<Rendering::GPUProxy::Camera::RasterInfo> cameraRasterInfos;
	cameras.reserve(m_CameraQuery.count());
	cameraRasterInfos.reserve(m_CameraQuery.count());
	m_CameraQuery.each([&cameras, &cameraRasterInfos](const Component::Camera& camera, const Component::Position& position, const Component::Rotation& rotation)
		{
			cameras.emplace_back(camera, position, rotation);
			cameraRasterInfos.emplace_back(camera.GetViewport(), camera.GetScizzorRect());
		});

	return std::make_tuple(std::move(meshes), std::move(cameras), std::move(cameraRasterInfos));
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
			if (mesh.GetMaterialID() == withID)
			{
				entity.remove<Component::Mesh>();
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
			for (const auto& [name, meshAsset] : allMeshes) {
				if (meshAsset.GetRenderID() == mesh.GetMeshID()) {
					wireframeRenderer.AddShape(Rendering::WireframeShape::Sphere(DXM::Vector3(0, 0, 1), DXM::Vector3::Transform(meshAsset.GetVertexData().Bounds.Center, DXM::Matrix::CreateTranslation(position)), meshAsset.GetVertexData().Bounds.Radius, 10));
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
