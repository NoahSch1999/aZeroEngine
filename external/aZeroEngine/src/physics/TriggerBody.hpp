#pragma once
#include <optional>
#include "physics/Body.hpp"

namespace aZero
{
	namespace Scene { class SceneNew; }
	namespace Physics
	{
		using BodyActivated_ResolveCallback = std::function<void()>;
		using BodyDeactivated_ResolveCallback = std::function<void()>;

		using ContactValidate_ResolveCallback = std::function<void(Physics::Body&, const JPH::RVec3Arg&, const JPH::CollideShapeResult&)>;
		using ContactAdded_ResolveCallback = std::function<void(Physics::Body&, const JPH::ContactManifold&, const JPH::ContactSettings&)>;
		using ContactPersisted_ResolveCallback = std::function<void(Physics::Body&, const JPH::ContactManifold&, const JPH::ContactSettings&)>;
		using ContactRemoved_ResolveCallback = std::function<void(const JPH::SubShapeIDPair&)>;

		class TriggerBody
		{
			friend class aZero::Scene::SceneNew;
		public:
			TriggerBody() = default;
			TriggerBody(const JPH::BodyCreationSettings& bodySettings)
				:m_TempBodySettings(bodySettings) {}
			TriggerBody(Physics::Body&& body)
				:m_Body(std::move(body)) {}

			Physics::Body& GetBody() { return m_Body; }

			void OnBodyActivated() { if (m_OnBodyActivated.has_value()) { m_OnBodyActivated.value()(); } }

			void OnBodyDeactivated() { if (m_OnBodyDeactivated.has_value()) { m_OnBodyDeactivated.value()(); } }

			void OnContactValidate(Physics::Body& other, const JPH::RVec3Arg& baseOffset, const JPH::CollideShapeResult& collisionResult) {
				if (m_OnContactValidate.has_value()) { m_OnContactValidate.value()(other, baseOffset, collisionResult); }
			}

			void OnContactAdded(Physics::Body& other, const JPH::ContactManifold& manifold, const JPH::ContactSettings& settings) {
				if (m_OnContactAdded.has_value()) { m_OnContactAdded.value()(other, manifold, settings); }
			}

			void OnContactPersisted(Physics::Body& other, const JPH::ContactManifold& manifold, const JPH::ContactSettings& settings) {
				if (m_OnContactPersisted.has_value()) { m_OnContactPersisted.value()(other, manifold, settings); }
			}

			void OnContactRemoved(const JPH::SubShapeIDPair& isubShapePair) { if (m_OnContactRemoved.has_value()) { m_OnContactRemoved.value()(isubShapePair); } }

			void SetOnBodyActivated(BodyActivated_ResolveCallback&& callback) { m_OnBodyActivated = std::move(callback); }
			void SetOnBodyDeactivated(BodyDeactivated_ResolveCallback&& callback) { m_OnBodyDeactivated = std::move(callback); }
			void SetOnContactValidate(ContactValidate_ResolveCallback&& callback) { m_OnContactValidate = std::move(callback); }
			void SetOnContactAdded(ContactAdded_ResolveCallback&& callback) { m_OnContactAdded = std::move(callback); }
			void SetOnContactPersisted(ContactPersisted_ResolveCallback&& callback) { m_OnContactPersisted = std::move(callback); }
			void SetOnContactRemoved(ContactRemoved_ResolveCallback&& callback) { m_OnContactRemoved = std::move(callback); }

		private:
			Physics::Body m_Body;
			JPH::BodyCreationSettings m_TempBodySettings;

			std::optional<BodyActivated_ResolveCallback> m_OnBodyActivated = std::optional<BodyActivated_ResolveCallback>();
			std::optional<BodyDeactivated_ResolveCallback> m_OnBodyDeactivated = std::optional<BodyDeactivated_ResolveCallback>();
			std::optional<ContactValidate_ResolveCallback> m_OnContactValidate = std::optional<ContactValidate_ResolveCallback>();
			std::optional<ContactAdded_ResolveCallback> m_OnContactAdded = std::optional<ContactAdded_ResolveCallback>();
			std::optional<ContactPersisted_ResolveCallback> m_OnContactPersisted = std::optional<ContactPersisted_ResolveCallback>();
			std::optional<ContactRemoved_ResolveCallback> m_OnContactRemoved = std::optional<ContactRemoved_ResolveCallback>();
		};
	}
}