#pragma once
#include "aZeroEngine/Engine.hpp"

inline void LoadAssets(
	aZero::Engine& engine, 
	aZero::Asset::Mesh& mesh,
	aZero::Asset::Material& material,
	aZero::Asset::Texture& albedo,
	aZero::Asset::Texture& normalMap)
{
	mesh.Load(engine.GetProjectDirectory() + MESH_ASSET_RELATIVE_PATH + "goblin.fbx");
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
	aZero::Scene::Scene& scene,
	aZero::Asset::Mesh& mesh,
	aZero::Asset::Material& material,
	const DXM::Vector2& windowDimensions)
{
	aZero::ECS::ComponentManagerDecl& ecsManager = scene.GetComponentManager();

	// Create mesh
	aZero::ECS::Entity meshEntity = scene.CreateEntity();
	scene.RenameEntity(meshEntity, "MeshEntity");
	ecsManager.AddComponent(meshEntity, aZero::ECS::StaticMeshComponent(&mesh, &material));

	ecsManager.GetComponent<aZero::ECS::TransformComponent>(meshEntity)
		->SetTransform(DXM::Matrix::CreateScale(0.5) * 
			DXM::Matrix::CreateRotationY(3.1415) * DXM::Matrix::CreateTranslation(0, -1.5, 5));

	scene.UpdateRenderState(meshEntity);

	// Create camera
	aZero::ECS::Entity cameraEntity = scene.CreateEntity();
	scene.RenameEntity(cameraEntity, "CameraEntity");

	aZero::ECS::CameraComponent cameraComponent;
	cameraComponent.m_TopLeft = { 0,0 };
	cameraComponent.m_Dimensions = windowDimensions;
	cameraComponent.m_NearPlane = 0.001f;
	cameraComponent.m_FarPlane = 1000.f;
	cameraComponent.m_Fov = 3.14f / 2.f;
	ecsManager.AddComponent(cameraEntity, cameraComponent);

	scene.UpdateRenderState(cameraEntity);
}

inline auto CreateRenderSurfaces(const aZero::Engine& engine, const DXM::Vector2& windowDimensions)
{
	aZero::Rendering::RenderTarget::Desc rtvDesc;
	rtvDesc.colorClearValue = { 0,0,0,0 };
	rtvDesc.dimensions = windowDimensions;
	rtvDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	aZero::Rendering::DepthTarget::Desc dsvDesc;
	dsvDesc.stencilClearValue = 0;
	dsvDesc.depthClearValue = 0;
	dsvDesc.dimensions = windowDimensions;
	dsvDesc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	return std::make_tuple(engine.CreateRenderTarget(rtvDesc, true), engine.CreateDepthStencilTarget(dsvDesc, true));
}