#pragma once
#include "EntityManager.h"
#include "ComponentManager.h"
#include "Core/aZeroECSWrap/Components/TransformComponent.h"
#include "Core/aZeroECSWrap/Components/StaticMeshComponent.h"
#include "Core/aZeroECSWrap/Components/TickComponent.h"
#include "Core/aZeroECSWrap/Components/CameraComponent.h"

#include "Core/aZeroECSWrap/Components/LightComponents.h"
//#include "SystemManager.h"

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