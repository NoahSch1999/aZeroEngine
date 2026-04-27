#include "Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "renderer/WireframeShapes.hpp"

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

	m_RootEntity = m_EntityManager.CreateEntity();
	m_Entities["RootEntity"] = m_RootEntity;
	m_Entity_To_Name[m_RootEntity.GetID()] = "RootEntity";
	m_ComponentManager.AddComponent(m_RootEntity, aZero::ECS::TransformComponent(m_RootEntity));
}

void aZero::Scene::SceneNew::AddRigidbody(ECS::RigidbodyComponent* rb)
{
	rb->m_Body = m_PhysicsWorld->CreateBody(rb->m_TempBodySettings, true);
}

void aZero::Scene::SceneNew::RemoveRigidbody(ECS::RigidbodyComponent* rb)
{
	m_PhysicsWorld->DestroyBody(rb->m_Body);
}

void aZero::Scene::SceneNew::QueueCollidersForRendering()
{
	auto& rbArray = m_ComponentManager.GetComponentArray<ECS::RigidbodyComponent>();
	for (auto& [name, entity] : m_Entities)
	{
		ECS::RigidbodyComponent* rbComp = rbArray.GetComponent(entity);
		if (rbComp)
		{
			auto [lock, body] = rbComp->m_Body.LockForRead();
			if (lock->Succeeded())
			{
				auto bounds = body->GetWorldSpaceBounds();
				m_PhysicsWorld->AddDebugAABB(Rendering::WireframeShape::AABB({ 0,1,0 }, Math::Convert(bounds.GetCenter()), Math::Convert(bounds.GetExtent())));
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

void aZero::Scene::SceneNew::OptimizePhysics()
{
	m_PhysicsWorld->OptimizeBroadPhase();
}

void aZero::Scene::SceneNew::ApplyPhysics()
{
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