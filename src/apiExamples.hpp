#pragma once
#include "aZeroEngine/Engine.hpp"
#include "graphics_api/resource/buffer/VertexBuffer.hpp"
#include "RenderWindow.hpp"

namespace Example {
	using namespace aZero;
	inline void LoadAssets(aZero::Engine& engine)
	{
		aZero::Asset::AssetManager& aManager = engine.GetAssetManager();
		aManager.LoadMesh(aZero::Asset::GetMeshDirectoryPath() + "cube2.fbx");

		aManager.LoadMaterial(aZero::Asset::GetMaterialDirectoryPath() + "TestMaterial.json");

		/*aManager.LoadTexture(aZero::Asset::GetTextureDirectoryPath() + "goblinAlbedo.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		aManager.LoadTexture(aZero::Asset::GetTextureDirectoryPath() + "goblinNormal.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		auto mat = aManager.CreateMaterial("TestMaterial");
		mat.value()->SetAlbedoTexture(aManager.GetTexture("goblinAlbedo").value());
		mat.value()->SetNormalMap(aManager.GetTexture("goblinNormal").value());
		engine.GetRenderer().UpdateRenderState(*mat.value());*/
	}

	inline void Setup(
		aZero::Engine& engine,
		aZero::Scene::Scene& scene,
		const DXM::Vector2& windowDimensions,
		Rendering::RenderTarget& rtv,
		Rendering::DepthStencilTarget& dsv,
		RenderWindow& window,
		Input::KeyboardListener& keyboardListener
	)
	{
		Example::LoadAssets(engine);
		aZero::Asset::AssetManager& aManager = engine.GetAssetManager();

		JPH::BoxShapeSettings meshShape(JPH::Vec3(1, 1, 1));
		JPH::BodyCreationSettings meshSettings(meshShape.Create().Get(), JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, aZero::Physics::Layers::DYNAMIC);
		meshSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
		meshSettings.mMassPropertiesOverride.mMass = 1.0f;
		for (int i = 1; i < 1; i++)
		{
			aZero::ECS::Entity meshEntity = scene.AddEntity();

			scene.AddComponent(meshEntity, aZero::ECS::StaticMeshComponent(aManager.GetMesh("cube2").value(), aManager.GetMaterial("TestMaterial").value()));

			if (scene.HasPhysics()) {
				meshSettings.mPosition = { (float)i + 2.f, 10, 0 };
				scene.AddComponent(meshEntity, aZero::ECS::RigidbodyComponent(meshSettings));
			}
			else
			{
				auto* tf = scene.m_ComponentManager.GetComponent<aZero::ECS::TransformComponent>(meshEntity);
				tf->SetTransform(DXM::Matrix::CreateTranslation((float)i + 4.f, 0, 0));
			}

			scene.MarkRenderStateDirty(meshEntity, aZero::Scene::Scene::ComponentFlag());
		}

		aZero::ECS::Entity floorEntity = scene.AddEntity();
		JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
		JPH::BodyCreationSettings floor_settings(floor_shape_settings.Create().Get(), JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, aZero::Physics::Layers::STATIC);
		scene.AddComponent(floorEntity, aZero::ECS::RigidbodyComponent(floor_settings));

		aZero::ECS::Entity resetEntity = scene.AddEntity();
		scene.RenameEntity(resetEntity, "resetEntity");
		meshSettings.mPosition = { 0, 50, 0 };
		scene.AddComponent(resetEntity, aZero::ECS::RigidbodyComponent(meshSettings));
		scene.AddComponent(resetEntity, aZero::ECS::StaticMeshComponent(aManager.GetMesh("cube2").value(), aManager.GetMaterial("TestMaterial").value()));
		scene.MarkRenderStateDirty(resetEntity, aZero::Scene::Scene::ComponentFlag());

		auto* rb = scene.m_ComponentManager.GetComponent<aZero::ECS::RigidbodyComponent>(resetEntity);
		if (rb)
		{
			rb->SetOnBodyActivated([resetEntity] {std::cout << resetEntity.GetID() << " activated!\n"; });
			rb->SetOnBodyDeactivated([resetEntity] {std::cout << resetEntity.GetID() << " deactivated!\n"; });
			rb->SetOnContactAdded([resetEntity, &scene](aZero::Physics::Body& body, const JPH::ContactManifold& man, const JPH::ContactSettings& sett) {
				std::cout << "Entity " << resetEntity.GetID() << " had contact with entity " << scene.GetEntityFromBody(body).value().GetID() << "\n";
				});
		}

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

			cameraComponent.m_RenderTarget = &rtv;
			cameraComponent.m_DepthStencilTarget = &dsv;

			scene.AddComponent(cameraEntity, cameraComponent);
			scene.AddComponent(cameraEntity, aZero::ECS::RigidbodyComponent(meshSettings));
			scene.AddComponent(cameraEntity, aZero::ECS::TransformComponent());

			scene.MarkRenderStateDirty(cameraEntity, aZero::Scene::Scene::ComponentFlag());
		}

		keyboardListener = window.GetDeviceManager().ListenKeyboard({
			[&window, &scene](const SDL_Event& event, Input::Keyboard& keyboard) {
				if (event.type == SDL_EVENT_KEY_DOWN)
				{
					if (event.key.key == SDLK_RETURN)
						window.Close();

					if (scene.HasPhysics())
					{
						if (event.key.key == SDLK_R)
						{
							auto resetEntity = scene.GetEntity("resetEntity").value();
							ECS::RigidbodyComponent& rbDropping = *scene.m_ComponentManager.GetComponent<ECS::RigidbodyComponent>(resetEntity);
							rbDropping.GetBody().SetPosition(JPH::Vec3(0, 2, 0), JPH::EActivation::Activate);
							rbDropping.GetBody().SetRotation(JPH::Quat::sEulerAngles(JPH::Vec3(0.5, 0.5, 0).Normalized()), JPH::EActivation::Activate);
						}
					}
				}
			},
			[](const SDL_Event& event, Input::Keyboard& keyboard) {}
			});
	}

	inline void ControlCamera(Input::KeyboardListener& listener, Scene::Scene& scene)
	{
		ECS::Entity camEnt = scene.GetEntity("CameraEntity").value();
		ECS::CameraComponent& cam = *scene.m_ComponentManager.GetComponent<ECS::CameraComponent>(camEnt);
		if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_W))
		{
			cam.m_Position += DXM::Vector3(0, 0, 0.03f);
		}

		if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_S))
		{
			cam.m_Position += DXM::Vector3(0, 0, -0.03f);
		}

		if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_D))
		{
			cam.m_Position += DXM::Vector3(-0.03f, 0, 0);
		}

		if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_A))
		{
			cam.m_Position += DXM::Vector3(0.03f, 0, 0);
		}

		if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_SPACE))
		{
			cam.m_Position += DXM::Vector3(0, 0.03f, 0);
		}

		if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_LSHIFT))
		{
			cam.m_Position += DXM::Vector3(0, -0.03f, 0);
		}

		ECS::RigidbodyComponent* camBody = scene.m_ComponentManager.GetComponent<ECS::RigidbodyComponent>(camEnt);
		if (camBody) {
			camBody->GetBody().SetPosition(Math::Convert(cam.m_Position), JPH::EActivation::Activate);
		}
		scene.MarkRenderStateDirty(camEnt, aZero::Scene::Scene::ComponentFlag());
	}
}