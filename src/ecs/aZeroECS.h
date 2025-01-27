#pragma once
#include "EntityManager.h"
#include "ComponentManager.h"
#include "components/TransformComponent.h"
#include "components/StaticMeshComponent.h"
#include "components/CameraComponent.h"
#include "components/LightComponents.h"

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
			CameraComponent> ComponentManagerDecl;
	}
}