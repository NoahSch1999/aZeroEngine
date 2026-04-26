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
	mesh.LoadFromFile("Goblin.fbx");
	engine.GetRenderer().UpdateRenderState(&mesh);

	albedo.Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinAlbedo.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	engine.GetRenderer().UpdateRenderState(&albedo);

	normalMap.Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinNormal.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
	engine.GetRenderer().UpdateRenderState(&normalMap);

	material.SetAlbedoTexture(&albedo);
	//material.SetNormalMap(&normalMap);
	engine.GetRenderer().UpdateRenderState(&material);
}

inline void CreateScene(
	aZero::Scene::SceneNew& scene,
	aZero::Asset::Mesh& mesh,
	aZero::Asset::Material& material,
	const DXM::Vector2& windowDimensions)
{
	aZero::ECS::ComponentManagerDecl& ecsManager = scene.m_ComponentManager;

	// Create mesh
	aZero::ECS::Entity meshEntity1 = scene.AddEntity();
	{
		scene.RenameEntity(meshEntity1, "MeshEntity");

		ecsManager.AddComponent(meshEntity1, aZero::ECS::StaticMeshComponent(&mesh, &material));

		ecsManager.GetComponent<aZero::ECS::TransformComponent>(meshEntity1)
			->SetTransform(DXM::Matrix::CreateRotationY(3.14) * DXM::Matrix::CreateTranslation(-10, -2, 4));

		scene.MarkRenderStateDirty(meshEntity1, aZero::Scene::SceneNew::ComponentFlag());
	}

	{
		for (int i = 1; i < 4; i++)
		{
			aZero::ECS::Entity meshEntity = scene.AddEntity();

			ecsManager.AddComponent(meshEntity, aZero::ECS::StaticMeshComponent(&mesh, &material));

			ecsManager.GetComponent<aZero::ECS::TransformComponent>(meshEntity)
				->SetTransform(DXM::Matrix::CreateRotationY(3.14) * DXM::Matrix::CreateTranslation(i * 3, -2, 4));

			scene.ParentEntity(meshEntity1, meshEntity);

			scene.MarkRenderStateDirty(meshEntity, aZero::Scene::SceneNew::ComponentFlag());
		}

		auto ent2 = scene.GetEntity("Entity_2");
		auto ent3 = scene.GetEntity("Entity_3");

		scene.ParentEntity(ent3, ent2.value());

		ecsManager.GetComponent<aZero::ECS::TransformComponent>(ent2.value())->SetTransform(DXM::Matrix::Identity);
	}

	// Create camera
	{
		aZero::ECS::Entity cameraEntity = scene.AddEntity();
		scene.RenameEntity(cameraEntity, "CameraEntity");

		aZero::ECS::CameraComponent cameraComponent;
		cameraComponent.m_TopLeft = { 0,0 };
		cameraComponent.m_Dimensions = { windowDimensions.x / 2.f, windowDimensions.y };
		cameraComponent.m_NearPlane = 0.001f;
		cameraComponent.m_FarPlane = 1000.f;
		cameraComponent.m_Fov = 3.14f / 2.f;
		cameraComponent.m_Layer = 1;
		cameraComponent.m_ClearRenderTarget = false;
		cameraComponent.m_ClearDepthTarget = false;
		cameraComponent.m_ClearStencilTarget = false;
		ecsManager.AddComponent(cameraEntity, cameraComponent);
		ecsManager.AddComponent(cameraEntity, aZero::ECS::TransformComponent());

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
		ecsManager.AddComponent(cameraEntity, cameraComponent);
		ecsManager.AddComponent(cameraEntity, aZero::ECS::TransformComponent());

		scene.MarkRenderStateDirty(cameraEntity, aZero::Scene::SceneNew::ComponentFlag());
	}
}