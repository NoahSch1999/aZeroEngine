#pragma once
#include <optional>
#include "physics/TriggerBody.hpp"

namespace aZero
{
	namespace Scene { class SceneNew; }
	namespace ECS
	{
		class RigidbodyComponent : public Physics::TriggerBody
		{
			friend class aZero::Scene::SceneNew;
		public:
			RigidbodyComponent() = default;
			RigidbodyComponent(const JPH::BodyCreationSettings& bodySettings)
				:TriggerBody(bodySettings) { }

		private:
		};
	}
}