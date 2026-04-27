#pragma once
#include "physics/Body.hpp"

namespace aZero
{
	namespace ECS
	{
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
		};
	}
}