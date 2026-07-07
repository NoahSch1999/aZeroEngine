#include "PhysicsWorld.hpp"
#include "renderer/WireframeRenderer.hpp"

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
PhysicsWorld
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

void aZero::Physics::PhysicsWorld::Init(JPH::JobSystemThreadPool& jobSystem, float updateFrequency)
{
	m_Body_activation_listener = std::make_unique<MyBodyActivationListener>();
	m_Contact_listener = std::make_unique<MyContactListener>();

	m_System.SetBodyActivationListener(m_Body_activation_listener.get());
	m_System.SetContactListener(m_Contact_listener.get());
	m_BodyInterface = &m_System.GetBodyInterface();
	m_Allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
	di_JobSystem = &jobSystem;
	m_UpdateFrequency = updateFrequency;
}

aZero::Physics::Body aZero::Physics::PhysicsWorld::CreateBody(const JPH::BodyCreationSettings& settings, bool addToPhysics)
{
	JPH::Body* body = m_BodyInterface->CreateBody(settings);
	if (body && addToPhysics)
	{
		m_BodyInterface->AddBody(body->GetID(), JPH::EActivation::Activate);
	}

	if (!body)
	{
		throw std::runtime_error("Cannot create new Jolt body"); // TODO: Handle if theres no more space for any more bodies
	}

	return Body(body->GetID(), *m_BodyInterface, m_System.GetBodyLockInterface());
}

void aZero::Physics::PhysicsWorld::AddBodiesToPhysics(const std::vector<Body>& bodies)
{
	std::vector<JPH::BodyID> tempBodies;
	tempBodies.reserve(bodies.size());
	for (const auto& body : bodies)
	{
		tempBodies.emplace_back(body.m_ID);
	}
	m_BodyInterface->AddBodiesPrepare(tempBodies.data(), tempBodies.size());
	m_BodyInterface->AddBodiesFinalize(tempBodies.data(), tempBodies.size(), JPH::BodyInterface::AddState(), JPH::EActivation::Activate);
}

void aZero::Physics::PhysicsWorld::FreezeBodies(const std::vector<Body>& bodies)
{
	std::vector<JPH::BodyID> tempBodies;
	tempBodies.reserve(bodies.size());
	for (const auto& body : bodies)
	{
		tempBodies.emplace_back(body.m_ID);
	}
	m_BodyInterface->RemoveBodies(tempBodies.data(), tempBodies.size());
}

void aZero::Physics::PhysicsWorld::DestroyBodies(const std::vector<Body>& bodies)
{
	std::vector<JPH::BodyID> tempBodies;
	tempBodies.reserve(bodies.size());
	for (const auto& body : bodies)
	{
		tempBodies.emplace_back(body.m_ID);
	}
	m_BodyInterface->RemoveBodies(tempBodies.data(), tempBodies.size());
	m_BodyInterface->DestroyBodies(tempBodies.data(), tempBodies.size());
}

void aZero::Physics::PhysicsWorld::ResetBodyEvents()
{
	m_Body_activation_listener->ResetEvents();
	m_Contact_listener->ResetEvents();
}

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MyBodyActivationListener
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

void aZero::Physics::PhysicsWorld::MyBodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData)
{
	std::unique_lock<std::mutex> lock(m_BodyActivationMutex);
	m_ActivatedEvents.emplace_back(inBodyUserData);
}

void aZero::Physics::PhysicsWorld::MyBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData)
{
	std::unique_lock<std::mutex> lock(m_BodyActivationMutex);
	m_DeactivatedEvents.emplace_back(inBodyUserData);
}

void aZero::Physics::PhysicsWorld::MyBodyActivationListener::ResetEvents()
{
	m_ActivatedEvents.clear();
	m_DeactivatedEvents.clear();
}

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
MyContactListener
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

JPH::ValidateResult aZero::Physics::PhysicsWorld::MyContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
	std::unique_lock<std::mutex> lock(m_ContactMutex);
	m_ContactValidateEvents.emplace_back(
		static_cast<uint64_t>(inBody1.GetUserData()),
		static_cast<uint64_t>(inBody2.GetUserData()),
		inBaseOffset,
		inCollisionResult);

	// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
	return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void aZero::Physics::PhysicsWorld::MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	std::unique_lock<std::mutex> lock(m_ContactMutex);
	m_ContactAddedEvents.emplace_back(
		static_cast<uint64_t>(inBody1.GetUserData()),
		static_cast<uint64_t>(inBody2.GetUserData()),
		inManifold,
		ioSettings);
}

void aZero::Physics::PhysicsWorld::MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	std::unique_lock<std::mutex> lock(m_ContactMutex);
	m_ContactPersistedEvents.emplace_back(
		static_cast<uint64_t>(inBody1.GetUserData()),
		static_cast<uint64_t>(inBody2.GetUserData()),
		inManifold,
		ioSettings);
}

void aZero::Physics::PhysicsWorld::MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
	std::unique_lock<std::mutex> lock(m_ContactMutex);
	m_ContactRemovedEvents.emplace_back(inSubShapePair);
}

void aZero::Physics::PhysicsWorld::MyContactListener::ResetEvents()
{
	m_ContactValidateEvents.clear();
	m_ContactAddedEvents.clear();
	m_ContactPersistedEvents.clear();
	m_ContactRemovedEvents.clear();
}