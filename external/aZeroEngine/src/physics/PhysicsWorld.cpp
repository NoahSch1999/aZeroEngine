#include "PhysicsWorld.hpp"
#include "renderer/WireframeRenderer.hpp"

void aZero::Physics::PhysicsWorld::Init(JPH::JobSystemThreadPool& jobSystem, Rendering::WireframeRenderer& colliderRenderer, float updateFrequency)
{
	m_Body_activation_listener = MyBodyActivationListener(m_ActivationCallbackExecutor);
	m_Contact_listener = MyContactListener(m_ContactCallbackExecutor);

	m_System.SetBodyActivationListener(&m_Body_activation_listener);
	m_System.SetContactListener(&m_Contact_listener);
	m_BodyInterface = &m_System.GetBodyInterface();
	m_Allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
	di_JobSystem = &jobSystem;
	di_ColliderRenderer = &colliderRenderer;
	m_UpdateFrequency = updateFrequency;
}

void aZero::Physics::PhysicsWorld::AddDebugSphere(const Rendering::WireframeShape::Sphere& sphere)
{
	di_ColliderRenderer->AddShape(sphere);
}

void aZero::Physics::PhysicsWorld::AddDebugAABB(const Rendering::WireframeShape::AABB& aabb)
{
	di_ColliderRenderer->AddShape(aabb);
}