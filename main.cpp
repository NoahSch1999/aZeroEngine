#include "aZeroEditor.hpp"
#include "RenderWindow.hpp"

#include "apiExamples.hpp"

#ifdef RUN_TESTS
#include "tests/Tests.hpp"
#endif

#if USE_DEBUG
#include <dxgidebug.h>
#endif

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

#define USE_DEBUG true

int main(int argc, char* argv[])
{
#if USE_DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	// CPU-side validation layer
	Microsoft::WRL::ComPtr<ID3D12Debug> d3d12Debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
		d3d12Debug->EnableDebugLayer();

	// GPU-side validation layer
	Microsoft::WRL::ComPtr<ID3D12Debug> dbContr0;
	Microsoft::WRL::ComPtr<ID3D12Debug1> dbContr1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&dbContr0));
	dbContr0->QueryInterface(IID_PPV_ARGS(&dbContr1));
	dbContr1->SetEnableGPUBasedValidation(true);

	Microsoft::WRL::ComPtr<IDXGIDebug> idxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&idxgiDebug));
#endif // DEBUG
	
	aZero::Window::Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

	// TODO: Take in width/height instead of vector2f in the entire project when specifying window dimensions
	try
	{
		// API Interfaces
		aZero::Engine engine(3);
		Rendering::Renderer& renderer = engine.GetRenderer();
		Audio::AudioEngine& audioEngine = engine.GetAudioEngine();
		Physics::PhysicsEngine& pEngine = engine.GetPhysicsEngine();
		//

#ifdef RUN_TESTS
		RunTests(engine);
#endif

		// Create your own implemented window and swapchain + input system
		//RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,800,600/*2560,1440*/ }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,2560,1440 }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		//

		// Create render surfaces
		auto [width, height] = window.GetClientDimensions();
		auto rtv = renderer.CreateRenderTarget(Rendering::RenderTarget::Desc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, { 0.3,0.3,0.3,1 }, true));
		auto dsv = renderer.CreateDepthStencilTarget(Rendering::DepthStencilTarget::Desc(width, height, 1, 0, true, true));
		//

		Asset::Mesh mesh(0);
		Asset::Material material(0);
		Asset::Texture albedo(0);
		Asset::Texture normalMap(1);
		LoadAssets(engine, mesh, material, albedo, normalMap);
		
		Scene::SceneNew scene = engine.CreateScene();
		CreateScene(scene, mesh, material, { (float)width, (float)height });
		//

		renderer.FlushFrameAllocations();

		ECS::Entity meshEntity = scene.GetEntity("MeshEntity").value();
		ECS::TransformComponent& tf = *scene.m_ComponentManager.GetComponent<ECS::TransformComponent>(meshEntity);
		
		ECS::Entity camEnt2 = scene.GetEntity("CameraEntity2").value();
		ECS::CameraComponent& cam2 = *scene.m_ComponentManager.GetComponent<ECS::CameraComponent>(camEnt2);
		cam2.m_RenderTarget = &rtv;
		cam2.m_DepthStencilTarget = &dsv;
		cam2.m_Position = { -10, 0, -15 };
		cam2.m_IsActive = false;
		scene.MarkRenderStateDirty(camEnt2, aZero::Scene::SceneNew::ComponentFlag());
		
		// Creatign rbs
		// Floor
		JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
		JPH::BodyCreationSettings floor_settings(floor_shape_settings.Create().Get(), JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Physics::Layers::NON_MOVING);
		auto ent = scene.GetEntity("Entity_1").value();
		scene.AddComponent(ent, ECS::RigidbodyComponent(floor_settings));

		// Cube 1
		JPH::BoxShapeSettings meshShape(JPH::Vec3(1, 1, 1));
		JPH::BodyCreationSettings meshSettings(meshShape.Create().Get(), JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Physics::Layers::MOVING);
		meshSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
		meshSettings.mMassPropertiesOverride.mMass = 1.0f;
		scene.AddComponent(meshEntity, ECS::RigidbodyComponent(meshSettings));

		// Cube 2
		meshSettings.mPosition = JPH::RVec3(0, 50, 0);
		meshSettings.mRotation = JPH::Quat::sEulerAngles(JPH::Vec3(0.5, 0.5, 0).Normalized());
		auto meshEntity3 = scene.GetEntity("Entity_3").value();
		scene.AddComponent(meshEntity3, ECS::RigidbodyComponent(meshSettings));
		//

		ECS::Entity camEnt = scene.GetEntity("CameraEntity").value();
		ECS::CameraComponent& cam = *scene.m_ComponentManager.GetComponent<ECS::CameraComponent>(camEnt);
		cam.m_RenderTarget = &rtv;
		cam.m_DepthStencilTarget = &dsv;
		JPH::BoxShapeSettings cameraCollider(JPH::Vec3(1, 1, 1));
		JPH::BodyCreationSettings cameraColliderSettings(cameraCollider.Create().Get(), JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Kinematic, Physics::Layers::MOVING);
		cameraColliderSettings.mPosition = JPH::RVec3(Math::Convert(cam.m_Position));
		scene.AddComponent(camEnt, ECS::RigidbodyComponent(cameraColliderSettings));
		scene.MarkRenderStateDirty(camEnt, aZero::Scene::SceneNew::ComponentFlag());

		bool showColliders = true;

		Input::KeyboardListener listener = window.GetDeviceManager().ListenKeyboard({
			[&window, meshEntity, &tf, &scene, &showColliders, &meshEntity3](const SDL_Event& event, Input::Keyboard& keyboard) {
				if (event.type == SDL_EVENT_KEY_DOWN)
				{
					if (event.key.key == SDLK_RETURN)
						window.Close();

					if (event.key.key == SDLK_P)
					{
						ECS::RigidbodyComponent& rbDropping = *scene.m_ComponentManager.GetComponent<ECS::RigidbodyComponent>(meshEntity3);
						rbDropping.m_Body.SetPosition(JPH::Vec3(0, 50, 0), JPH::EActivation::Activate);
						rbDropping.m_Body.SetRotation(JPH::Quat::sEulerAngles(JPH::Vec3(0.5, 0.5, 0).Normalized()), JPH::EActivation::Activate);
					}

					if (event.key.key == SDLK_T)
					{
						showColliders = !showColliders;
					}
				}
			},
			[](const SDL_Event& event, Input::Keyboard& keyboard) { }
			});

		int frame = 0;

		while (window.IsOpen())
		{
			window.Update();

			frame++;
			// Declares start of new frame and loops until the new frame can be rendered
			while (!engine.TryBeginFrame())
			{
				// Do some stuff while waiting, ex. queue physics calcs on a seperate thread
			}

			if (frame % 3 == 0)
			{
				scene.UpdatePhysics(true);
			}

			// Camera controls
			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_W))
			{
				cam.m_Position += DXM::Vector3(0, 0, 0.01f);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_S))
			{
				cam.m_Position += DXM::Vector3(0, 0, -0.03f);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_D))
			{
				cam.m_Position += DXM::Vector3(-0.01f, 0, 0);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_A))
			{
				cam.m_Position += DXM::Vector3(0.01f, 0, 0);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_SPACE))
			{
				cam.m_Position += DXM::Vector3(0, 0.01f, 0);
			}

			if (listener.GetDevice()->IsKeyDown(SDL_SCANCODE_LSHIFT))
			{
				cam.m_Position += DXM::Vector3(0, -0.01f, 0);
			}

			{
				ECS::RigidbodyComponent& camBody = *scene.m_ComponentManager.GetComponent<ECS::RigidbodyComponent>(camEnt);
				camBody.m_Body.SetPosition(Math::Convert(cam.m_Position), JPH::EActivation::Activate);
			}

			scene.MarkRenderStateDirty(camEnt, aZero::Scene::SceneNew::ComponentFlag());
			//

			// Rendering the scene
			renderer.Render(scene);

			if (showColliders)
			{
				scene.QueueCollidersForRendering();
				engine.DrawColliders(cam, rtv, dsv);
			}

			renderer.CopyRenderTargetToSwapChain(window.GetSwapChain(), rtv);

			// Declares end of current frame
			engine.EndFrame();

			window.Present();
		}

		renderer.FlushGPU();
	}
	catch (std::invalid_argument& e)
	{
		printf(e.what());
		DebugBreak();
	}

	aZero::Window::Shutdown();
#if USE_DEBUG
	idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL));
#endif
	return 0;
}