#pragma once
#include "JoltInclude.hpp"

namespace aZero
{
	namespace Physics
	{
		class PhysicsWorld;

		class Body 
		{
			friend class PhysicsWorld;
		public:
			Body() = default;

			std::tuple<std::unique_ptr<JPH::BodyLockWrite>, JPH::Body*> LockForWrite()
			{
				std::unique_ptr<JPH::BodyLockWrite> lock = std::make_unique<JPH::BodyLockWrite>(*m_LockingInterface, m_ID);
				return std::make_tuple<std::unique_ptr<JPH::BodyLockWrite>, JPH::Body*>(std::move(lock), m_LockingInterface->TryGetBody(m_ID));
			}

			std::tuple<std::unique_ptr<JPH::BodyLockRead>, JPH::Body*> LockForRead()
			{
				std::unique_ptr<JPH::BodyLockRead> lock = std::make_unique<JPH::BodyLockRead>(*m_LockingInterface, m_ID);
				return std::make_tuple<std::unique_ptr<JPH::BodyLockRead>, JPH::Body*>(std::move(lock), m_LockingInterface->TryGetBody(m_ID));
			}

			// Oh god...but this is for the best (thanks AI for speeding up the process):

			void SetLinearAndAngularVelocity(JPH::Vec3Arg& inLinearVelocity, JPH::Vec3Arg& inAngularVelocity) {
				m_BodyInterface->SetLinearAndAngularVelocity(m_ID, inLinearVelocity, inAngularVelocity);
			}

			void GetLinearAndAngularVelocity(JPH::Vec3& outLinearVelocity, JPH::Vec3& outAngularVelocity) const {
				m_BodyInterface->GetLinearAndAngularVelocity(m_ID, outLinearVelocity, outAngularVelocity);
			}

			void SetLinearVelocity(JPH::Vec3Arg inLinearVelocity) {
				m_BodyInterface->SetLinearVelocity(m_ID, inLinearVelocity);
			}

			JPH::Vec3 GetLinearVelocity() const {
				return m_BodyInterface->GetLinearVelocity(m_ID);
			}

			void AddLinearVelocity(JPH::Vec3Arg inLinearVelocity) {
				m_BodyInterface->AddLinearVelocity(m_ID, inLinearVelocity);
			}

			void AddLinearAndAngularVelocity(JPH::Vec3Arg inLinearVelocity, JPH::Vec3Arg inAngularVelocity) {
				m_BodyInterface->AddLinearAndAngularVelocity(m_ID, inLinearVelocity, inAngularVelocity);
			}

			void SetAngularVelocity(JPH::Vec3Arg inAngularVelocity) {
				m_BodyInterface->SetAngularVelocity(m_ID, inAngularVelocity);
			}

			JPH::Vec3 GetAngularVelocity() const {
				return m_BodyInterface->GetAngularVelocity(m_ID);
			}

			JPH::Vec3 GetPointVelocity(JPH::RVec3Arg inPoint) const {
				return m_BodyInterface->GetPointVelocity(m_ID, inPoint);
			}

			void SetPositionRotationAndVelocity(JPH::RVec3Arg inPosition, JPH::QuatArg inRotation, JPH::Vec3Arg inLinearVelocity, JPH::Vec3Arg inAngularVelocity) {
				m_BodyInterface->SetPositionRotationAndVelocity(m_ID, inPosition, inRotation, inLinearVelocity, inAngularVelocity);
			}

			const JPH::PhysicsMaterial* GetMaterial(const JPH::SubShapeID& inSubShapeID) const {
				return m_BodyInterface->GetMaterial(m_ID, inSubShapeID);
			}

			void ActivateBody() {
				m_BodyInterface->ActivateBody(m_ID);
			}

			void DeactivateBody() {
				m_BodyInterface->DeactivateBody(m_ID);
			}

			bool IsActive() const {
				return m_BodyInterface->IsActive(m_ID);
			}

			void ResetSleepTimer() {
				m_BodyInterface->ResetSleepTimer(m_ID);
			}

			void SetObjectLayer(JPH::ObjectLayer inLayer) {
				m_BodyInterface->SetObjectLayer(m_ID, inLayer);
			}

			JPH::ObjectLayer GetObjectLayer() const {
				return m_BodyInterface->GetObjectLayer(m_ID);
			}

			void SetPositionAndRotation(JPH::RVec3Arg inPosition, JPH::QuatArg inRotation, JPH::EActivation inActivationMode) {
				m_BodyInterface->SetPositionAndRotation(m_ID, inPosition, inRotation, inActivationMode);
			}

			void SetPositionAndRotationWhenChanged(JPH::RVec3Arg inPosition, JPH::QuatArg inRotation, JPH::EActivation inActivationMode) {
				m_BodyInterface->SetPositionAndRotationWhenChanged(m_ID, inPosition, inRotation, inActivationMode);
			}

			void GetPositionAndRotation(JPH::RVec3& outPosition, JPH::Quat& outRotation) const {
				m_BodyInterface->GetPositionAndRotation(m_ID, outPosition, outRotation);
			}

			void SetPosition(JPH::RVec3Arg inPosition, JPH::EActivation inActivationMode) {
				m_BodyInterface->SetPosition(m_ID, inPosition, inActivationMode);
			}

			JPH::RVec3 GetPosition() const {
				return m_BodyInterface->GetPosition(m_ID);
			}

			JPH::RVec3 GetCenterOfMassPosition() const {
				return m_BodyInterface->GetCenterOfMassPosition(m_ID);
			}

			void SetRotation(JPH::QuatArg inRotation, JPH::EActivation inActivationMode) {
				m_BodyInterface->SetRotation(m_ID, inRotation, inActivationMode);
			}

			JPH::Quat GetRotation() const {
				return m_BodyInterface->GetRotation(m_ID);
			}

			JPH::RMat44 GetWorldTransform() const {
				return m_BodyInterface->GetWorldTransform(m_ID);
			}

			JPH::RMat44 GetCenterOfMassTransform() const {
				return m_BodyInterface->GetCenterOfMassTransform(m_ID);
			}

			void AddForce(JPH::Vec3Arg inForce, JPH::EActivation inActivationMode = JPH::EActivation::Activate) {
				m_BodyInterface->AddForce(m_ID, inForce, inActivationMode);
			}

			void AddForce(JPH::Vec3Arg inForce, JPH::RVec3Arg inPoint, JPH::EActivation inActivationMode = JPH::EActivation::Activate) {
				m_BodyInterface->AddForce(m_ID, inForce, inPoint, inActivationMode);
			}

			void AddTorque(JPH::Vec3Arg inTorque, JPH::EActivation inActivationMode = JPH::EActivation::Activate) {
				m_BodyInterface->AddTorque(m_ID, inTorque, inActivationMode);
			}

			void AddForceAndTorque(JPH::Vec3Arg inForce, JPH::Vec3Arg inTorque, JPH::EActivation inActivationMode = JPH::EActivation::Activate) {
				m_BodyInterface->AddForceAndTorque(m_ID, inForce, inTorque, inActivationMode);
			}

			void AddImpulse(JPH::Vec3Arg inImpulse) {
				m_BodyInterface->AddImpulse(m_ID, inImpulse);
			}

			void AddImpulse(JPH::Vec3Arg inImpulse, JPH::RVec3Arg inPoint) {
				m_BodyInterface->AddImpulse(m_ID, inImpulse, inPoint);
			}

			void AddAngularImpulse(JPH::Vec3Arg inAngularImpulse) {
				m_BodyInterface->AddAngularImpulse(m_ID, inAngularImpulse);
			}

			bool ApplyBuoyancyImpulse(JPH::RVec3Arg inSurfacePosition, JPH::Vec3Arg inSurfaceNormal, float inBuoyancy, float inLinearDrag, float inAngularDrag, JPH::Vec3Arg inFluidVelocity, JPH::Vec3Arg inGravity, float inDeltaTime) {
				return m_BodyInterface->ApplyBuoyancyImpulse(m_ID, inSurfacePosition, inSurfaceNormal, inBuoyancy, inLinearDrag, inAngularDrag, inFluidVelocity, inGravity, inDeltaTime);
			}

			JPH::EBodyType GetBodyType() const {
				return m_BodyInterface->GetBodyType(m_ID);
			}

			void SetMotionType(JPH::EMotionType inMotionType, JPH::EActivation inActivationMode) {
				m_BodyInterface->SetMotionType(m_ID, inMotionType, inActivationMode);
			}

			JPH::EMotionType GetMotionType() const {
				return m_BodyInterface->GetMotionType(m_ID);
			}

			void SetMotionQuality(JPH::EMotionQuality inMotionQuality) {
				m_BodyInterface->SetMotionQuality(m_ID, inMotionQuality);
			}

			JPH::EMotionQuality GetMotionQuality() const {
				return m_BodyInterface->GetMotionQuality(m_ID);
			}

			void SetRestitution(float inRestitution) {
				m_BodyInterface->SetRestitution(m_ID, inRestitution);
			}

			float GetRestitution() const {
				return m_BodyInterface->GetRestitution(m_ID);
			}

			void SetFriction(float inFriction) {
				m_BodyInterface->SetFriction(m_ID, inFriction);
			}

			float GetFriction() const {
				return m_BodyInterface->GetFriction(m_ID);
			}

			void SetGravityFactor(float inGravityFactor) {
				m_BodyInterface->SetGravityFactor(m_ID, inGravityFactor);
			}

			float GetGravityFactor() const {
				return m_BodyInterface->GetGravityFactor(m_ID);
			}

			void SetMaxLinearVelocity(float inLinearVelocity) {
				m_BodyInterface->SetMaxLinearVelocity(m_ID, inLinearVelocity);
			}

			float GetMaxLinearVelocity() const {
				return m_BodyInterface->GetMaxLinearVelocity(m_ID);
			}

			void SetMaxAngularVelocity(float inAngularVelocity) {
				m_BodyInterface->SetMaxAngularVelocity(m_ID, inAngularVelocity);
			}

			float GetMaxAngularVelocity() const {
				return m_BodyInterface->GetMaxAngularVelocity(m_ID);
			}

			void SetUseManifoldReduction(bool inUseReduction) {
				m_BodyInterface->SetUseManifoldReduction(m_ID, inUseReduction);
			}

			bool GetUseManifoldReduction() const {
				return m_BodyInterface->GetUseManifoldReduction(m_ID);
			}

			void SetIsSensor(bool inIsSensor) {
				m_BodyInterface->SetIsSensor(m_ID, inIsSensor);
			}

			bool IsSensor() const {
				return m_BodyInterface->IsSensor(m_ID);
			}

			void SetCollisionGroup(const JPH::CollisionGroup& inCollisionGroup) {
				m_BodyInterface->SetCollisionGroup(m_ID, inCollisionGroup);
			}

			const JPH::CollisionGroup& GetCollisionGroup() const {
				return m_BodyInterface->GetCollisionGroup(m_ID);
			}

		private:
			Body(JPH::BodyID id, JPH::BodyInterface& bodyInterface, const JPH::BodyLockInterfaceLocking& lockingInterface)
				:m_ID(id), m_BodyInterface(&bodyInterface), m_LockingInterface(&lockingInterface) {}

			JPH::BodyID m_ID;
			JPH::BodyInterface* m_BodyInterface = nullptr;
			const JPH::BodyLockInterfaceLocking* m_LockingInterface = nullptr;
		};
	}
}