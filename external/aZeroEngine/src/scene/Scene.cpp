#include "Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "renderer/WireframeRenderer.hpp"

aZero::Scene::SceneNew::SceneNew(Physics::PhysicsEngine& physicsEngine)
	:m_Proxy(std::make_unique<SceneProxy>()), m_PhysicsWorld(std::make_unique<Physics::PhysicsWorld>())
{
	physicsEngine.CreateWorld(*m_PhysicsWorld.get());

	m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::PointLightComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::SpotLightComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::DirectionalLightComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::CameraComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::RigidbodyComponent>().Init(1000);
	m_ComponentManager.GetComponentArray<ECS::ColliderComponent>().Init(1000);

	m_RootEntity = m_EntityManager.CreateEntity();
	m_Entities["RootEntity"] = m_RootEntity;
	m_Entity_To_Name[m_RootEntity.GetID()] = "RootEntity";
	m_ComponentManager.AddComponent(m_RootEntity, aZero::ECS::TransformComponent(m_RootEntity));
}

void aZero::Scene::SceneNew::AddRigidbody(const ECS::Entity& entity, ECS::RigidbodyComponent* rb)
{
	/*
		Cast the entity ID to 64bit and then store the entity ID in the first (most significant) 32bits by shifting the first 32bits to the left 32bits.
		Example when entity ID is 2:
			0b00000000000000000000000000000010(<-upper 32bit starts)00000000000000000000000000000000(<- lower 32bit starts)
			Then it can be extracted like this by shifting them back 32bits:
				static_cast<uint32_t>((userData) >> 32); => 0b00000000000000000000000000000010
	*/
	rb->m_TempBodySettings.mUserData = static_cast<uint64_t>(entity.GetID()) << 32;

	rb->m_Body = m_PhysicsWorld->CreateBody(rb->m_TempBodySettings, true);
	auto [lock, body] = rb->m_Body.LockForWrite();

	m_BodyID_To_Entity[rb->m_Body.GetBodyID()] = entity;
}

void aZero::Scene::SceneNew::RemoveRigidbody(ECS::RigidbodyComponent* rb)
{
	m_BodyID_To_Entity.erase(rb->m_Body.GetBodyID());
	m_PhysicsWorld->DestroyBody(rb->m_Body);
}

void aZero::Scene::SceneNew::AddRigidbody(const ECS::Entity& entity, aZero::Physics::Body& body, JPH::BodyCreationSettings& tempBodySettings)
{
	tempBodySettings.mUserData = static_cast<uint64_t>(entity.GetID()) << 32;

	body = m_PhysicsWorld->CreateBody(tempBodySettings, true);
	m_BodyID_To_Entity[body.GetBodyID()] = entity;
}

void aZero::Scene::SceneNew::RemoveRigidbody(const aZero::Physics::Body& body)
{
	m_BodyID_To_Entity.erase(body.GetBodyID());
	m_PhysicsWorld->DestroyBody(body);
}

void aZero::Scene::SceneNew::AddDebugDrawArguments(Rendering::WireframeRenderer& wireframeRenderer, bool showColliders, bool showMeshBounds)
{
	auto& rbArray = m_ComponentManager.GetComponentArray<ECS::RigidbodyComponent>();
	auto& tfArray = m_ComponentManager.GetComponentArray<ECS::TransformComponent>();
	auto& smArray = m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>();
	auto& colliderArray = m_ComponentManager.GetComponentArray<ECS::ColliderComponent>();
	for (auto& [name, entity] : m_Entities)
	{
		if (showColliders) {
			ECS::RigidbodyComponent* rbComp = rbArray.GetComponent(entity);
			if (rbComp)
			{
				auto [lock, body] = rbComp->m_Body.LockForRead();
				if (lock->Succeeded())
				{
					auto bounds = body->GetWorldSpaceBounds();

					auto* shape = body->GetShape();
					//const JPH::BoxShape* boxShape = dynamic_cast<const JPH::BoxShape*>(body->GetShape()); // Why crash with dynamic cast? Answer: RTTI OFF :(
					if (body->GetShape()->GetSubType() == JPH::EShapeSubType::Box)
					{
						const JPH::BoxShape* boxShape = static_cast<const JPH::BoxShape*>(body->GetShape());
						wireframeRenderer.AddShape(Rendering::WireframeShape::OBB(DXM::Vector3(0, 1, 1), Math::Convert(body->GetPosition()), Math::Convert(body->GetRotation()), Math::Convert(boxShape->GetHalfExtent())));
					}
					
					// TODO: Implement support for other shapes
				}
			}

			ECS::ColliderComponent* colliderComp = colliderArray.GetComponent(entity);
			if (colliderComp)
			{
				auto& colliders = colliderComp->m_Colliders;
				for (auto& collider : colliders)
				{
					auto [lock, body] = collider.m_Body.LockForRead();
					if (lock->Succeeded())
					{
						auto bounds = body->GetWorldSpaceBounds();

						if (body->GetShape()->GetSubType() == JPH::EShapeSubType::Box)
						{
							const JPH::BoxShape* boxShape = reinterpret_cast<const JPH::BoxShape*>(body->GetShape());
							wireframeRenderer.AddShape(Rendering::WireframeShape::OBB(DXM::Vector3(0, 1, 1), Math::Convert(body->GetPosition()), Math::Convert(body->GetRotation()), Math::Convert(boxShape->GetHalfExtent())));
						}

						// TODO: Implement support for other shapes
					}
				}
				
			}
		}

		if (showMeshBounds)
		{
			ECS::TransformComponent* tfComp = tfArray.GetComponent(entity);
			ECS::StaticMeshComponent* smComp = smArray.GetComponent(entity);
			if (tfComp && smComp)
			{
				const auto& meshData = smComp->GetMesh()->GetVertexData();
				wireframeRenderer.AddShape(Rendering::WireframeShape::Sphere(DXM::Vector3(1, 0, 0), DXM::Vector3::Transform(meshData.Bounds.Center, tfComp->GetTransform()), meshData.Bounds.Radius, 10));

				// Too poor performance...
				/*for (const auto& meshlet : meshData.Meshlets)
				{
					wireframeRenderer.AddShape(Rendering::WireframeShape::Sphere(DXM::Vector3(1, 0, 0), DXM::Vector3::Transform(meshlet.Bounds.Center, tfComp->GetTransform()), meshlet.Bounds.Radius, 10));
				}*/
			}
		}
	}
}

void aZero::Scene::SceneNew::UpdatePhysics(bool applyImmediate)
{
	m_PhysicsWorld->Update();
	if (applyImmediate) {
		this->ApplyPhysics();
	}
}

void aZero::Scene::SceneNew::ResolveCollisionEvents()
{
	auto& rbArray = m_ComponentManager.GetComponentArray<ECS::RigidbodyComponent>();
	for (const auto& event : m_PhysicsWorld->GetBodyActivatedEvents()) {
		ECS::RigidbodyComponent* rbComp = rbArray.GetComponent(event.EntityID);
		if (rbComp->m_OnBodyActivated.has_value()) {
			rbComp->m_OnBodyActivated.value()();
		}
	}

	for (const auto& event : m_PhysicsWorld->GetBodyDeactivatedEvents()) {
		ECS::RigidbodyComponent* rbComp = rbArray.GetComponent(event.EntityID);
		if (rbComp->m_OnBodyDeactivated.has_value()) {
			rbComp->m_OnBodyDeactivated.value()();
		}
	}

	// For both bodies in a collision we call the collision functions with the other body as an argument
	for (const auto& event : m_PhysicsWorld->GetContactValidateEvents()) {
		ECS::RigidbodyComponent* firstBody = rbArray.GetComponent(event.FirstEntityID);
		ECS::RigidbodyComponent* secondBody = rbArray.GetComponent(event.SecondEntityID);

		if (firstBody->m_OnContactValidate.has_value()) {
			firstBody->m_OnContactValidate.value()(secondBody->m_Body, event.InBaseOffset, *event.InCollisionResult.get());
		}

		if (secondBody->m_OnContactValidate.has_value()) {
			secondBody->m_OnContactValidate.value()(firstBody->m_Body, event.InBaseOffset, *event.InCollisionResult.get());
		}
	}

	for (const auto& event : m_PhysicsWorld->GetContactAddedEvents()) {
		ECS::RigidbodyComponent* firstBody = rbArray.GetComponent(event.FirstEntityID);
		ECS::RigidbodyComponent* secondBody = rbArray.GetComponent(event.SecondEntityID);

		if (firstBody->m_OnContactAdded.has_value()) {
			firstBody->m_OnContactAdded.value()(secondBody->m_Body, event.ContactManifold, event.ContactSettings);
		}

		if (secondBody->m_OnContactAdded.has_value()) {
			secondBody->m_OnContactAdded.value()(firstBody->m_Body, event.ContactManifold, event.ContactSettings);
		}
	}

	for (const auto& event : m_PhysicsWorld->GetContactPersistedEvents()) {
		ECS::RigidbodyComponent* firstBody = rbArray.GetComponent(event.FirstEntityID);
		ECS::RigidbodyComponent* secondBody = rbArray.GetComponent(event.SecondEntityID);

		if (firstBody->m_OnContactPersisted.has_value()) {
			firstBody->m_OnContactPersisted.value()(secondBody->m_Body, event.ContactManifold, event.ContactSettings);
		}

		if (secondBody->m_OnContactPersisted.has_value()) {
			secondBody->m_OnContactPersisted.value()(firstBody->m_Body, event.ContactManifold, event.ContactSettings);
		}
	}

	for (const auto& event : m_PhysicsWorld->GetContactRemovedEvents()) {
		ECS::RigidbodyComponent* firstBody = rbArray.GetComponent(m_BodyID_To_Entity.at(event.InSubShapePair.GetBody1ID()));
		ECS::RigidbodyComponent* secondBody = rbArray.GetComponent(m_BodyID_To_Entity.at(event.InSubShapePair.GetBody2ID()));

		if (firstBody->m_OnContactRemoved.has_value()) {
			firstBody->m_OnContactRemoved.value()(event.InSubShapePair);
		}

		if (secondBody->m_OnContactRemoved.has_value()) {
			secondBody->m_OnContactRemoved.value()(event.InSubShapePair);
		}
	}
	//

	m_PhysicsWorld->ResetBodyEvents();
}

void aZero::Scene::SceneNew::OptimizePhysics()
{
	m_PhysicsWorld->OptimizeBroadPhase();
}

void aZero::Scene::SceneNew::ApplyPhysics()
{
	this->ResolveCollisionEvents();

	auto& rbArray = m_ComponentManager.GetComponentArray<ECS::RigidbodyComponent>();
	auto& tfArray = m_ComponentManager.GetComponentArray<ECS::TransformComponent>();
	for (auto& [name, entity] : m_Entities)
	{
		ECS::RigidbodyComponent* rbComp = rbArray.GetComponent(entity);
		if (rbComp)
		{
			auto [lock, body] = rbComp->m_Body.LockForRead();
			if (lock->Succeeded())
			{
				ECS::TransformComponent* tfComp = tfArray.GetComponent(entity);
				if (tfComp)
				{
					tfComp->SetTransform(DXM::Matrix::CreateFromQuaternion(Math::Convert(body->GetRotation())) * DXM::Matrix::CreateTranslation(Math::Convert(body->GetPosition())));
					this->MarkRenderStateDirty(entity, aZero::Scene::SceneNew::ComponentFlag());
				}
			}
		}
	}
}