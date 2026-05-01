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

void aZero::Scene::SceneNew::AddCollidersForRendering(Rendering::WireframeRenderer& wireframeRenderer)
{
	auto& rbArray = m_ComponentManager.GetComponentArray<ECS::RigidbodyComponent>();
	auto& tfArray = m_ComponentManager.GetComponentArray<ECS::TransformComponent>();
	auto& smArray = m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>();
	for (auto& [name, entity] : m_Entities)
	{
		ECS::RigidbodyComponent* rbComp = rbArray.GetComponent(entity);
		if (rbComp)
		{
			auto [lock, body] = rbComp->m_Body.LockForRead();
			if (lock->Succeeded())
			{
				auto bounds = body->GetWorldSpaceBounds();

				const JPH::BoxShape* x = static_cast<const JPH::BoxShape* const>(body->GetShape());
				auto rot = body->GetRotation();
				auto trans = body->GetPosition();
				auto halfExt = x->GetHalfExtent();
				Rendering::WireframeShape::OBB obb = { DXM::Vector3(0,1,1), Math::Convert(trans), Math::Convert(rot), Math::Convert(halfExt) };
				wireframeRenderer.AddShape(obb);
			}
		}

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