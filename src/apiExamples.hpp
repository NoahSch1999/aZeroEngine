#pragma once
#include "Engine.hpp"
#include "RenderWindow.hpp"

namespace Example {
	using namespace aZero;
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
		std::string meshName = "Mesh";
		aZero::Asset::AssetManager& aManager = engine.GetAssetManager();

		auto res = FBX::LoadFBX(aZero::Asset::GetMeshDirectoryPath() + "goblin.fbx");

		//aManager.LoadMesh(aZero::Asset::GetMeshDirectoryPath() + meshName + ".fbx");
		aManager.AddMesh(res.value().Meshes[0]);
		aManager.LoadMaterial(aZero::Asset::GetMaterialDirectoryPath() + "TestMaterial.json");
		

		JPH::BoxShapeSettings boxShape(JPH::Vec3(1, 1, 1));
		JPH::BodyCreationSettings boxSettings(boxShape.Create().Get(), JPH::RVec3(0.0, 100.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, aZero::Physics::Layers::DYNAMIC);
		boxSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
		boxSettings.mMassPropertiesOverride.mMass = 1.0f;

		{
			flecs::entity entMesh = scene.GetEntityWorld().entity().is_a(scene.GetRigidbodyStaticMeshPrefab());
			entMesh.set_name("Mesh");
			entMesh.set(Component::Mesh(*aManager.GetMesh(meshName).value(), *aManager.GetMaterial("TestMaterial").value()));
			Component::Rigidbody& rb = entMesh.get_mut<Component::Rigidbody>();
			rb.SetCreationSettings(boxSettings);
			scene.RegisterToPhysics(entMesh);
		}

		{
			flecs::entity entCam = scene.GetEntityWorld().entity().is_a(scene.GetCameraPrefab());
			entCam.set_name("Camera");
			auto [xWin, yWin] = window.GetClientDimensions();
			entCam.set<Component::Position>({ 0,0,-2 });
			entCam.set<Component::Rotation>({ 0,0,0 });
			entCam.set<Component::Camera>({ 3.14 / 2.f, 0.001f, 1000.f, true, { 0,0 }, { (float)xWin, (float)yWin } });
		}

		{
			for (int i = 0; i < 1; i+=3) {
				for (int j = 0; j < 1500; j += 3)
				{
					flecs::entity ent = scene.GetEntityWorld().entity().is_a(scene.GetStaticMeshPrefab());
					std::string name = std::string("Mesh") + std::to_string(i) + std::to_string(j);
					ent.set_name(name.c_str());
					ent.set(Component::Mesh(*aManager.GetMesh(meshName).value(), *aManager.GetMaterial("TestMaterial").value()));
					ent.set<Component::Position>(DXM::Vector3(i, 2, j));
					ent.set<Component::Rotation>(DXM::Vector3(0, 3.14, 0));
					//Component::Rigidbody& rb = ent.get_mut<Component::Rigidbody>();
					//rb.SetCreationSettings(boxSettings);
					//scene.RegisterToPhysics(ent);
					//ent.get_mut<Component::Rigidbody>().GetBody().SetPosition(Math::Convert(DXM::Vector3(i, 2, j)), JPH::EActivation::Activate);
				}
			}
		}

		{
			flecs::entity floorEnt = scene.GetEntityWorld().entity().is_a(scene.GetRigidbodyStaticMeshPrefab());
			floorEnt.remove<Component::Mesh>();
			JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
			JPH::BodyCreationSettings floor_settings(floor_shape_settings.Create().Get(), JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, aZero::Physics::Layers::STATIC);
			Component::Rigidbody& rb = floorEnt.get_mut<Component::Rigidbody>();
			rb.SetCreationSettings(floor_settings);
			scene.RegisterToPhysics(floorEnt);
			floorEnt.set_name("Floor");

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
						//flecs::entity ent = scene.GetEntityWorld().lookup("Mesh00");
						//ent.get_mut<Component::Rigidbody>().GetBody().SetPosition(Math::Convert(DXM::Vector3(0, 100, 0)), JPH::EActivation::Activate);
					}
				}
			}
		},
		[](const SDL_Event& event, Input::Keyboard& keyboard) {}
			});
	}

	void ControlCamera(aZero::Scene::Scene& scene, Input::KeyboardListener& listener)
	{
		flecs::entity ent = scene.GetEntityWorld().lookup("Camera");

		if (scene.HasPhysics() && ent.has<Component::Rigidbody>())
		{
			Component::Rigidbody& rb = ent.get_mut<Component::Rigidbody>();

			DXM::Vector3 moveDirection = DXM::Vector3::Zero;
			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_W))
			{
				moveDirection.z += 1.f;
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_S))
			{
				moveDirection.z -= 1.f;
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_D))
			{
				moveDirection.x -= 1.f;
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_A))
			{
				moveDirection.x += 1.f;
			}

			moveDirection.Normalize();
			moveDirection = moveDirection * 5.f;

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_SPACE))
			{
				moveDirection.y += 10.f;
			}

			rb.GetBody().AddForce(Math::Convert(moveDirection));
			rb.GetBody().SetRotation(JPH::Quat::sIdentity(), JPH::EActivation::Activate);
		}
		else
		{
			Component::Position cam = ent.get<Component::Position>();

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_W))
			{
				cam += DXM::Vector3(0, 0, 0.03f);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_S))
			{
				cam += DXM::Vector3(0, 0, -0.03f);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_D))
			{
				cam += DXM::Vector3(-0.03f, 0, 0);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_A))
			{
				cam += DXM::Vector3(0.03f, 0, 0);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_SPACE))
			{
				cam += DXM::Vector3(0, 0.03f, 0);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_LSHIFT))
			{
				cam += DXM::Vector3(0, -0.03f, 0);
			}

			ent.set(cam);
		}
	}
}