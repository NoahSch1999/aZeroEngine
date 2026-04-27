#include "PhysicsEngine.hpp"
#include "PhysicsWorld.hpp"
#include "renderer/WireframeRenderer.hpp"

aZero::Physics::PhysicsEngine::PhysicsEngine(ID3D12DeviceX* device, IDxcCompilerX& compiler)
{
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();
	m_JobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 4);
	m_BroadPhaseLayerInterface = std::make_unique<BPLayerInterfaceImpl>();
	m_ObjectVsBroadPhaseLayerInterface = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
	m_ObjectLayerPairInterface = std::make_unique<ObjectLayerPairFilterImpl>();

	m_ColliderRenderer = std::make_unique<Rendering::WireframeRenderer>(device, compiler);
}

aZero::Physics::PhysicsEngine::~PhysicsEngine()
{
	if (m_JobSystem.get())
	{
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}

void aZero::Physics::PhysicsEngine::CreateWorld(aZero::Physics::PhysicsWorld& world, float updateFrequency, uint32_t maxBodies, uint32_t maxBodyPairs, uint32_t maxContactConstraints, uint32_t maxBodyMutexes)
{
	world.m_System.Init(maxBodies, maxBodyMutexes, maxBodyPairs, maxContactConstraints, *m_BroadPhaseLayerInterface.get(), *m_ObjectVsBroadPhaseLayerInterface.get(), *m_ObjectLayerPairInterface.get());
	world.Init(*m_JobSystem.get(), *m_ColliderRenderer.get(), updateFrequency);
}

aZero::Rendering::WireframeRenderer& aZero::Physics::PhysicsEngine::GetColliderRenderer() { return *m_ColliderRenderer.get(); }