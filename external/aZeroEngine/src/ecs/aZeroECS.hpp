#pragma once
#include "EntityManager.hpp"
#include "ComponentManager.hpp"
#include "components/TransformComponent.hpp"
#include "components/StaticMeshComponent.hpp"
#include "components/CameraComponent.hpp"
#include "components/LightComponents.hpp"
#include "components/RigidbodyComponent.hpp"

namespace aZero
{
	namespace ECS
	{
		typedef ComponentManager<
			TransformComponent, 
			StaticMeshComponent,
			DirectionalLightComponent,
			PointLightComponent, 
			SpotLightComponent, 
			CameraComponent,
			RigidbodyComponent
		> ComponentManagerDecl;
	}
}