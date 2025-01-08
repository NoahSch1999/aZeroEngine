#pragma once
#include "EntityManager.h"
#include "ComponentManager.h"
#include "Core/aZeroECSWrap/Components/TransformComponent.h"
#include "Core/aZeroECSWrap/Components/StaticMeshComponent.h"
#include "Core/aZeroECSWrap/Components/PointLightComponent.h"
#include "Core/aZeroECSWrap/Components/SpotLightComponent.h"
#include "Core/aZeroECSWrap/Components/DirectionalLightComponent.h"
#include "Core/aZeroECSWrap/Components/TickComponent.h"
#include "Core/aZeroECSWrap/Components/CameraComponent.h"
//#include "SystemManager.h"

namespace aZero
{
	namespace ECS
	{
		typedef ComponentManager<TransformComponent, StaticMeshComponent, PointLightComponent, CameraComponent> ComponentManagerDecl;
	}
}