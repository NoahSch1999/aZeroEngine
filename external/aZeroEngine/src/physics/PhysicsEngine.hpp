#pragma once
#include "JoltInclude.hpp"
#include "PhysicsWorld.hpp"

namespace aZero
{
	namespace Rendering
	{
		class WireframeRenderer;
	}

	namespace Physics
	{
		class PhysicsWorld;

		namespace Layers
		{
			static constexpr JPH::ObjectLayer NON_MOVING = 0;
			static constexpr JPH::ObjectLayer MOVING = 1;
			static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
		};

		namespace BroadPhaseLayers
		{
			static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
			static constexpr JPH::BroadPhaseLayer MOVING(1);
			static constexpr uint32_t NUM_LAYERS(2);
		};

		// BroadPhaseLayerInterface implementation
		// This defines a mapping between object and broadphase layers.
		class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
		{
		public:
			BPLayerInterfaceImpl()
			{
				// Create a mapping table from object to broad phase layer
				m_ObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
				m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
			}
		
			virtual uint32_t GetNumBroadPhaseLayers() const override
			{
				return BroadPhaseLayers::NUM_LAYERS;
			}
		
			virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
			{
				//JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
				return m_ObjectToBroadPhase[inLayer];
			}
		
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
			virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
			{
				switch ((JPH::BroadPhaseLayer::Type)inLayer)
				{
				case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
				case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
				default:													JPH_ASSERT(false); return "INVALID";
				}
			}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

		private:
			JPH::BroadPhaseLayer m_ObjectToBroadPhase[Layers::NUM_LAYERS];
		};

		class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
		{
		public:
			virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
			{
				switch (inLayer1)
				{
				case Layers::NON_MOVING:
					return inLayer2 == BroadPhaseLayers::MOVING;
				case Layers::MOVING:
					return true;
				default:
					JPH_ASSERT(false);
					return false;
				}
			}
		};

		class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
		{
		public:
			virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
			{
				switch (inObject1)
				{
				case Layers::NON_MOVING:
					return inObject2 == Layers::MOVING; // Non moving only collides with moving
				case Layers::MOVING:
					return true; // Moving collides with everything
				default:
					JPH_ASSERT(false);
					return false;
				}
			}
		};

		class PhysicsEngine
		{
		public:
			PhysicsEngine(ID3D12DeviceX* device, IDxcCompilerX& compiler);

			~PhysicsEngine();

			void CreateWorld(PhysicsWorld& world, float updateFrequency = 1.0f / 60.f, uint32_t maxBodies = 1024, uint32_t maxBodyPairs = 1024, uint32_t maxContactConstraints = 1024, uint32_t maxBodyMutexes = 0);

			Rendering::WireframeRenderer& GetColliderRenderer();

		private:
			std::unique_ptr<JPH::JobSystemThreadPool> m_JobSystem;
			std::unique_ptr<BPLayerInterfaceImpl> m_BroadPhaseLayerInterface;
			std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> m_ObjectVsBroadPhaseLayerInterface;
			std::unique_ptr<ObjectLayerPairFilterImpl> m_ObjectLayerPairInterface;
			std::unique_ptr<Rendering::WireframeRenderer> m_ColliderRenderer;
		};
	}
}