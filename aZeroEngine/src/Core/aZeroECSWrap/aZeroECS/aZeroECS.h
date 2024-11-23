#pragma once
#include "EntityManager.h"
#include "ComponentManager.h"
#include "Core/aZeroECSWrap/Components/TransformComponent.h"
#include "Core/aZeroECSWrap/Components/StaticMeshComponent.h"
#include "Core/aZeroECSWrap/Components/PointLightComponent.h"
#include "Core/aZeroECSWrap/Components/SpotLightComponent.h"
#include "Core/aZeroECSWrap/Components/DirectionalLightComponent.h"
#include "Core/aZeroECSWrap/Components/TickComponent.h"
//#include "SystemManager.h"

namespace aZero
{
	const unsigned int MAX_ENTITIES = 1000; // Replace to handle resize of entity max

	namespace ECS
	{
		typedef ComponentManager<TransformComponent, StaticMeshComponent, PointLightComponent, SpotLightComponent, DirectionalLightComponent, TickComponent_Interface> ComponentManagerDecl;
	}
}