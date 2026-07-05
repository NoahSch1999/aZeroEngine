#pragma once
#include "src/RenderWindow.hpp"
#include "Engine.hpp"
#include "EditorGUI.hpp"

inline std::pair<int, int> spiral(int n)
{
	if (n == 0)
		return { 0, 0 };

	int layer = std::ceil((std::sqrt(n + 1) - 1) / 2);
	int sideLength = layer * 2;
	int maxValue = (2 * layer + 1) * (2 * layer + 1) - 1;
	int offset = maxValue - n;

	if (offset < sideLength)
		return { layer - offset, -layer };

	offset -= sideLength;
	if (offset < sideLength)
		return { -layer, -layer + offset };

	offset -= sideLength;
	if (offset < sideLength)
		return { -layer + offset, layer };

	offset -= sideLength;
	return { layer, layer - offset };
}

namespace aZero::Editor
{
	class Editor
	{
	public:
		Editor()
		{
			m_Engine = std::make_unique<Engine>(PROJECT_DIRECTORY, 3);
			m_MainWindow = std::make_unique<RenderWindow>(Window::WindowDesc("aZero Engine", { 0,0,1920,1080 }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE, m_Engine->GetAssetManager().GetAssetDirectory<Asset::Texture>() + std::string("editorIcon.png")), m_Engine->GetRenderer());
			
			m_KeyboardListener = m_MainWindow->GetDeviceManager().ListenKeyboard({
				[this](const SDL_Event& event, Input::Keyboard& keyboard) {
				if (event.type == SDL_EVENT_KEY_DOWN)
				{
					if (event.key.key == SDLK_RETURN)
						m_MainWindow->Close();

					if (m_CurrentScene->HasPhysics())
					{
						if (event.key.key == SDLK_R)
						{
							flecs::entity ent = m_CurrentScene->GetEntityWorld().lookup("Mesh");
							ent.get_mut<Component::Rigidbody>().GetBody().SetPosition(Math::Convert(DXM::Vector3(0, 100, 0)), JPH::EActivation::Activate);
						}
					}
					if (event.key.key == SDLK_ESCAPE)
					{
						this->m_Gui.m_ShowEditorGUI = !this->m_Gui.m_ShowEditorGUI;
					}
				}
			},
			[](const SDL_Event& event, Input::Keyboard& keyboard) {}
					});

			m_FrameRtv = m_Engine->GetRenderer().CreateRenderTarget(Rendering::RenderTarget::Desc(DXGI_FORMAT_R8G8B8A8_UNORM, 1920, 1080, { 0.3,0.3,0.3,1 }, true));
			m_FrameDsv = m_Engine->GetRenderer().CreateDepthStencilTarget(Rendering::DepthStencilTarget::Desc(1920, 1080, 1, 0, true, true));

			aZero::ImGui_Wrapper::Init(m_Engine->GetRenderer(), m_MainWindow->GetSDLWindow());
			m_Gui = aZero::Editor::GUI::EditorGUI(m_MainWindow->GetDeviceManager(), m_Engine->GetRenderer().GetWireframeRenderer());

			this->SetupSceneTest();

			m_Engine->GetRenderer().FlushFrameAllocations();
		}

		~Editor()
		{
			m_Engine->GetRenderer().FlushRenderCommands();

			/*m_CurrentScene.reset();
			m_MainWindow.reset();
			m_Engine.reset();*/

			ImGui_ImplDX12_Shutdown();
			ImGui_ImplSDL3_Shutdown();
			ImGui::DestroyContext();
		}

		void Run()
		{
			while (m_MainWindow->IsOpen())
			{
				this->Update();
			}
		}

		void Update()
		{
			m_MainWindow->Update();

			while (!m_Engine->GetRenderer().TryBeginFrame())
			{
				// Do some stuff while waiting, ex. queue physics calcs on a seperate thread
			}

			aZero::ImGui_Wrapper::BeginFrame();

			if (m_Frame % 3 == 2)
			{
				m_CurrentScene->UpdatePhysics(true);
			}

			m_Gui.Update(*m_CurrentScene.get());

			this->CameraUpdate();

			m_Engine->GetRenderer().Render(*m_CurrentScene.get(), m_FrameRtv, m_FrameDsv);

			flecs::entity camEnt = m_CurrentScene->GetEntityWorld().lookup("EditorCamera");
			m_Engine->GetRenderer().GetWireframeRenderer().Render(camEnt.get<Component::Camera>(), camEnt.get<Component::Position>(), camEnt.get<Component::Rotation>(), m_FrameRtv, m_FrameDsv);

			m_Gui.Render(m_Engine->GetRenderer(), m_FrameRtv);

			m_Engine->GetRenderer().CopyRenderTargetToSwapChain(m_MainWindow->GetSwapChain(), m_FrameRtv);

			m_Engine->GetRenderer().EndFrame();

			m_MainWindow->Present();
			m_Frame++;
		}

	private:
		void CameraUpdate()
		{
			flecs::entity ent = m_CurrentScene->GetEntityWorld().lookup("EditorCamera");

			if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_0))
			{
				m_CurrentScene->MarkStaticMeshesDirty();
			}

			if (m_CurrentScene->HasPhysics() && ent.has<Component::Rigidbody>())
			{
				Component::Rigidbody& rb = ent.get_mut<Component::Rigidbody>();

				DXM::Vector3 moveDirection = DXM::Vector3::Zero;
				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_W))
				{
					moveDirection.z += 1.f;
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_S))
				{
					moveDirection.z -= 1.f;
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_D))
				{
					moveDirection.x -= 1.f;
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_A))
				{
					moveDirection.x += 1.f;
				}

				moveDirection.Normalize();
				moveDirection = moveDirection * 5.f;

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_SPACE))
				{
					moveDirection.y += 10.f;
				}

				rb.GetBody().AddForce(Math::Convert(moveDirection));
				rb.GetBody().SetRotation(JPH::Quat::sIdentity(), JPH::EActivation::Activate);
			}
			else
			{
				Component::Position cam = ent.get<Component::Position>();

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_W))
				{
					cam += DXM::Vector3(0, 0, 0.03f);
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_S))
				{
					cam += DXM::Vector3(0, 0, -0.03f);
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_D))
				{
					cam += DXM::Vector3(-0.03f, 0, 0);
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_A))
				{
					cam += DXM::Vector3(0.03f, 0, 0);
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_SPACE))
				{
					cam += DXM::Vector3(0, 0.03f, 0);
				}

				if (m_KeyboardListener.GetDevice()->IsKeyDown(SDL_SCANCODE_LSHIFT))
				{
					cam += DXM::Vector3(0, -0.03f, 0);
				}

				ent.set(cam);
			}
		}

		void SetupSceneTest()
		{
			m_CurrentScene = std::make_unique<Scene::Scene>(m_Engine->GetPhysicsEngine());

			auto& assetManager = m_Engine->GetAssetManager();
			assetManager.RegisterScene(m_CurrentScene.get());

			auto loadedFBX = FBX::LoadFBX(assetManager.GetAssetDirectory<Asset::Mesh>() + "multimat.fbx");
			if (!loadedFBX.has_value()) { throw; }

			auto meshObj = assetManager.Create<Asset::Mesh>("mesh", loadedFBX.value().Meshes[0]);
			auto matObj = assetManager.Create<Asset::Material>("mat", Asset::MaterialData(assetManager.GetAssetDirectory<Asset::Material>() + "TestMaterial.json"));

			JPH::BoxShapeSettings boxShape(JPH::Vec3(1, 1, 1));
			auto shapeRes = boxShape.Create().Get();
			JPH::BodyCreationSettings boxSettings(shapeRes, JPH::RVec3(0.0, 100.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, aZero::Physics::Layers::DYNAMIC);
			boxSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
			boxSettings.mMassPropertiesOverride.mMass = 1.0f;
			{
				flecs::entity entMesh = m_CurrentScene->GetEntityWorld().entity().is_a(m_CurrentScene->GetStaticMeshPrefab());
				entMesh.set<Component::Rigidbody>(boxSettings);
				entMesh.set_name("Mesh");
				entMesh.set(Component::Mesh(*meshObj, *matObj));
			}

			{
				flecs::entity entCam = m_CurrentScene->GetEntityWorld().entity().is_a(m_CurrentScene->GetCameraPrefab());
				entCam.set_name("EditorCamera");
				auto [xWin, yWin] = m_MainWindow->GetClientDimensions();
				entCam.set<Component::Position>({ 0,0,-2 });
				entCam.set<Component::Rotation>({ 0,0,0 });
				entCam.set<Component::Camera>({ 3.14 / 2.f, 0.001f, 1000.f, true, { 0,0 }, { (float)xWin, (float)yWin } });
			}

			{
				for (int j = 0; j < 500; j++)
				{
					flecs::entity ent = m_CurrentScene->GetEntityWorld().entity().is_a(m_CurrentScene->GetStaticMeshPrefab());
					std::string name = std::string("Mesh") + std::to_string(j);
					ent.set_name(name.c_str());
					ent.set(Component::Mesh(*meshObj, *matObj));
					auto [x, z] = spiral(j);
					ent.set<Component::Position>(DXM::Vector3(x * 3, 2, z * 3));
					//ent.set<Component::Position>(DXM::Vector3(0, 2, j));
					ent.set<Component::Rotation>(DXM::Vector3(0, 3.14, 0));
					ent.add<Component::Static>();
				}
			}

			{
				flecs::entity floorEnt = m_CurrentScene->GetEntityWorld().entity().is_a(m_CurrentScene->GetStaticMeshPrefab());
				floorEnt.remove<Component::Mesh>();
				JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
				JPH::BodyCreationSettings floor_settings(floor_shape_settings.Create().Get(), JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, aZero::Physics::Layers::STATIC);
				Component::Rigidbody rigidbodyFloor;
				rigidbodyFloor.SetCreationSettings(floor_settings);
				floorEnt.set<Component::Rigidbody>(rigidbodyFloor);
				floorEnt.set_name("Floor");
			}
		}

		std::unique_ptr<Engine> m_Engine;
		std::unique_ptr<RenderWindow> m_MainWindow;
		Rendering::RenderTarget m_FrameRtv;
		Rendering::DepthStencilTarget m_FrameDsv;
		aZero::Editor::GUI::EditorGUI m_Gui;
		std::unique_ptr<Scene::Scene> m_CurrentScene;
		uint64_t m_Frame = 0;
		Input::KeyboardListener m_KeyboardListener;
	};
}