#pragma once
#include "aZeroEngine/Engine.hpp"
#include "graphics_api/resource/buffer/VertexBuffer.hpp"

inline void LoadAssets(
	aZero::Engine& engine, 
	aZero::Asset::Mesh& mesh,
	aZero::Asset::Material& material,
	aZero::Asset::Texture& albedo,
	aZero::Asset::Texture& normalMap)
{
	mesh.LoadFromFile("goblin.fbx");
	engine.GetRenderer().UpdateRenderState(&mesh);

	albedo.Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinAlbedo.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	engine.GetRenderer().UpdateRenderState(&albedo);

	normalMap.Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinNormal.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
	engine.GetRenderer().UpdateRenderState(&normalMap);

	material.SetAlbedoTexture(&albedo);
	material.SetNormalMap(&normalMap);
	engine.GetRenderer().UpdateRenderState(&material);
}

inline void CreateScene(
	aZero::Scene::SceneNew& scene,
	aZero::Asset::Mesh& mesh,
	aZero::Asset::Material& material,
	const DXM::Vector2& windowDimensions)
{
	{
		JPH::BoxShapeSettings meshShape(JPH::Vec3(1, 1, 1));
		JPH::BodyCreationSettings meshSettings(meshShape.Create().Get(), JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, aZero::Physics::Layers::MOVING);
		meshSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
		meshSettings.mMassPropertiesOverride.mMass = 1.0f;

		for (int i = 1; i < 4; i++)
		{
			aZero::ECS::Entity meshEntity = scene.AddEntity();

			scene.AddComponent(meshEntity, aZero::ECS::StaticMeshComponent(&mesh, &material));

			meshSettings.mPosition = { (float)i + 0.5f, 10, 0 };
			scene.AddComponent(meshEntity, aZero::ECS::RigidbodyComponent(meshSettings));

			scene.MarkRenderStateDirty(meshEntity, aZero::Scene::SceneNew::ComponentFlag());
		}

		aZero::ECS::Entity floorEntity = scene.AddEntity();
		JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
		JPH::BodyCreationSettings floor_settings(floor_shape_settings.Create().Get(), JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, aZero::Physics::Layers::NON_MOVING);
		scene.AddComponent(floorEntity, aZero::ECS::RigidbodyComponent(floor_settings));

		aZero::ECS::Entity resetEntity = scene.AddEntity();
		scene.RenameEntity(resetEntity, "resetEntity");
		meshSettings.mPosition = { 0, 50, 0 };
		scene.AddComponent(resetEntity, aZero::ECS::RigidbodyComponent(meshSettings));
		scene.AddComponent(resetEntity, aZero::ECS::StaticMeshComponent(&mesh, &material));
		scene.MarkRenderStateDirty(resetEntity, aZero::Scene::SceneNew::ComponentFlag());
	}

	// Create camera
	{
		aZero::ECS::Entity cameraEntity = scene.AddEntity();
		scene.RenameEntity(cameraEntity, "CameraEntity");

		aZero::ECS::CameraComponent cameraComponent;
		cameraComponent.m_TopLeft = { 0,0 };
		cameraComponent.m_Dimensions = { windowDimensions.x /*/ 2.f*/, windowDimensions.y };
		cameraComponent.m_NearPlane = 0.001f;
		cameraComponent.m_FarPlane = 1000.f;
		cameraComponent.m_Fov = 3.14f / 2.f;
		cameraComponent.m_Layer = 1;
		/*cameraComponent.m_ClearRenderTarget = false;
		cameraComponent.m_ClearDepthTarget = false;
		cameraComponent.m_ClearStencilTarget = false;*/
		scene.AddComponent(cameraEntity, cameraComponent);
		scene.AddComponent(cameraEntity, aZero::ECS::TransformComponent());

		scene.MarkRenderStateDirty(cameraEntity, aZero::Scene::SceneNew::ComponentFlag());
	}

	{
		aZero::ECS::Entity cameraEntity = scene.AddEntity();
		scene.RenameEntity(cameraEntity, "CameraEntity2");

		aZero::ECS::CameraComponent cameraComponent;
		cameraComponent.m_TopLeft = { windowDimensions.x / 2.f, 0 };
		cameraComponent.m_Dimensions = { windowDimensions.x / 2.f, windowDimensions.y };
		cameraComponent.m_NearPlane = 0.001f;
		cameraComponent.m_FarPlane = 1000.f;
		cameraComponent.m_Fov = 3.14f / 2.f;
		cameraComponent.m_Layer = 0;
		scene.AddComponent(cameraEntity, cameraComponent);
		scene.AddComponent(cameraEntity, aZero::ECS::TransformComponent());

		scene.MarkRenderStateDirty(cameraEntity, aZero::Scene::SceneNew::ComponentFlag());
	}
}