#include "PhysicsEngine.hpp"
#include "PhysicsWorld.hpp"

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
PhysicsEngine
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/
aZero::Physics::PhysicsEngine::PhysicsEngine(ID3D12DeviceX* device, IDxcCompilerX& compiler)
{
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();
	m_JobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 4);
	m_BroadPhaseLayerInterface = std::make_unique<BPLayerInterfaceImpl>();
	m_ObjectVsBroadPhaseLayerInterface = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
	m_ObjectLayerPairInterface = std::make_unique<ObjectLayerPairFilterImpl>();
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
	world.Init(*m_JobSystem.get(), updateFrequency);
}

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
BPLayerInterfaceImpl
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

aZero::Physics::BPLayerInterfaceImpl::BPLayerInterfaceImpl()
{
	// Create a mapping table from object to broad phase layer
	m_ObjectToBroadPhase[Layers::STATIC] = BroadPhaseLayers::STATIC;
	m_ObjectToBroadPhase[Layers::DYNAMIC] = BroadPhaseLayers::DYNAMIC;
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* aZero::Physics::BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
{
	switch ((JPH::BroadPhaseLayer::Type)inLayer)
	{
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::STATIC:	return "STATIC";
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::DYNAMIC:		return "DYNAMIC";
	default:													JPH_ASSERT(false); return "INVALID";
	}
}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ObjectVsBroadPhaseLayerFilterImpl
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

bool aZero::Physics::ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
{
	switch (inLayer1)
	{
	case Layers::STATIC:
		return inLayer2 == BroadPhaseLayers::DYNAMIC;
	case Layers::DYNAMIC:
		return true;
	default:
		JPH_ASSERT(false);
		return false;
	}
}

/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ObjectLayerPairFilterImpl
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

bool aZero::Physics::ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
{
	switch (inObject1)
	{
	case Layers::STATIC:
		return inObject2 == Layers::DYNAMIC; // Non moving only collides with moving
	case Layers::DYNAMIC:
		return true; // Moving collides with everything
	default:
		JPH_ASSERT(false);
		return false;
	}
}