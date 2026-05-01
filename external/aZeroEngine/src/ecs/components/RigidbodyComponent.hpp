#pragma once
#include <optional>
#include "physics/Body.hpp"

namespace aZero
{
	namespace ECS
	{
		using BodyActivated_ResolveCallback = std::function<void()>;
		using BodyDeactivated_ResolveCallback = std::function<void()>;

		using ContactValidate_ResolveCallback = std::function<void(Physics::Body&, JPH::RVec3Arg, const JPH::CollideShapeResult&)>;
		using ContactAdded_ResolveCallback = std::function<void(Physics::Body&, const JPH::ContactManifold&, const JPH::ContactSettings&)>;
		using ContactPersisted_ResolveCallback = std::function<void(Physics::Body&, const JPH::ContactManifold&, const JPH::ContactSettings&)>;
		using ContactRemoved_ResolveCallback = std::function<void(const JPH::SubShapeIDPair&)>;

		class RigidbodyComponent
		{
		private:

		public:
			Physics::Body m_Body;
			JPH::BodyCreationSettings m_TempBodySettings;
			RigidbodyComponent() = default;
			RigidbodyComponent(const JPH::BodyCreationSettings& bodySettings)
				:m_TempBodySettings(bodySettings)
			{

			}

			std::optional<BodyActivated_ResolveCallback> m_OnBodyActivated = std::optional<BodyActivated_ResolveCallback>();
			std::optional<BodyDeactivated_ResolveCallback> m_OnBodyDeactivated = std::optional<BodyDeactivated_ResolveCallback>();
			std::optional<ContactValidate_ResolveCallback> m_OnContactValidate = std::optional<ContactValidate_ResolveCallback>();
			std::optional<ContactAdded_ResolveCallback> m_OnContactAdded = std::optional<ContactAdded_ResolveCallback>();
			std::optional<ContactPersisted_ResolveCallback> m_OnContactPersisted = std::optional<ContactPersisted_ResolveCallback>();
			std::optional<ContactRemoved_ResolveCallback> m_OnContactRemoved = std::optional<ContactRemoved_ResolveCallback>();
		};
	}
}