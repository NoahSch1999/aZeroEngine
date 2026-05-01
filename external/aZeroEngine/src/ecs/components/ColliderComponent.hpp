#pragma once
#include <optional>
#include "physics/TriggerBody.hpp"

namespace aZero
{
	namespace Scene { class SceneNew; }
	namespace Physics { class PhysicsWorld; }
	namespace ECS
	{
		class Entity;
		class ColliderComponent
		{
			friend class aZero::Scene::SceneNew;
		public:
			ColliderComponent() = default;

			// TODO: Make these not manually requiring scene and owning entity as args
			void AddCollider(Scene::SceneNew& scene, const ECS::Entity& entity, JPH::BodyCreationSettings& bodySettings);
			void RemoveCollider(Scene::SceneNew& scene, uint32_t index);
			const std::vector<Physics::TriggerBody>& GetColliders() const { return m_Colliders; }
		private:
			std::vector<Physics::TriggerBody> m_Colliders;
		};
	}
}