#include "ColliderComponent.hpp"
#include "physics/PhysicsWorld.hpp"
#include "scene/Scene.hpp"
#include "ecs/Entity.hpp"

void aZero::ECS::ColliderComponent::AddCollider(Scene::Scene& scene, const ECS::Entity& entity, JPH::BodyCreationSettings& bodySettings)
{
	Physics::Body newBody;
	bodySettings.mIsSensor = true;
	scene.AddRigidbody(entity, newBody, bodySettings);
	m_Colliders.push_back(std::move(newBody));
}

void aZero::ECS::ColliderComponent::RemoveCollider(Scene::Scene& scene, uint32_t index)
{
	scene.RemoveRigidbody(m_Colliders[index].GetBody());
	m_Colliders.erase(m_Colliders.begin() + index);
}