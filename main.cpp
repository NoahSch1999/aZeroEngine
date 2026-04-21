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
		//

#ifdef RUN_TESTS
		RunTests(engine);
#endif

		// Create your own implemented window and swapchain + input system
		RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,800,600/*2560,1440*/ }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		//RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,2560,1440 }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		//

		// Create render surfaces
		auto [width, height] = window.GetClientDimensions();
		auto [rtv, dsv] = CreateRenderSurfaces(engine, { (float)width, (float)height });
		//

		Asset::Mesh mesh(0);
		Asset::Material material(0);
		Asset::Texture albedo(0);
		Asset::Texture normalMap(1);
		LoadAssets(engine, mesh, material, albedo, normalMap);
		
		Scene::SceneNew scene;
		CreateScene(scene, mesh, material, { (float)width, (float)height });
		//

		renderer.FlushFrameAllocations();

		ECS::Entity meshEntity = scene.GetEntity("MeshEntity").value();
		ECS::TransformComponent& tf = *scene.m_ComponentManager.GetComponent<ECS::TransformComponent>(meshEntity);

		ECS::Entity camEnt = scene.GetEntity("CameraEntity").value();
		ECS::CameraComponent& cam = *scene.m_ComponentManager.GetComponent<ECS::CameraComponent>(camEnt);

		Input::KeyboardListener listener = window.GetDeviceManager().ListenKeyboard({
			[&window, meshEntity, &tf, &scene](const SDL_Event& event, Input::Keyboard& keyboard) {
				if (event.key.key == SDLK_RETURN)
					window.Close();
				if (event.key.key == SDLK_R)
				{
					tf.SetTransform(DXM::Matrix::CreateRotationY(3.14) * DXM::Matrix::CreateTranslation(0, -2, 4));
					scene.MarkRenderStateDirty(meshEntity, aZero::Scene::SceneNew::ComponentFlag());
				}
			},
			[](const SDL_Event& event, Input::Keyboard& keyboard) { }
			});

		;

		while (window.IsOpen())
		{
			window.Update();

			// Declares start of new frame and loops until the new frame can be rendered
			while (!renderer.BeginFrame())
			{
				// Do some stuff while waiting, ex. queue physics calcs on a seperate thread
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
			scene.MarkRenderStateDirty(camEnt, aZero::Scene::SceneNew::ComponentFlag());

			tf.SetTransform(DXM::Matrix::CreateRotationY(0.001f) * tf.GetTransform());
			scene.MarkRenderStateDirty(meshEntity, aZero::Scene::SceneNew::ComponentFlag());
			//

			// Rendering the scene
			renderer.Render(scene, &rtv, &dsv);

			renderer.CopyTextureToTexture(window.GetCurrentBackbuffer(), rtv.GetTexture().GetResource());

			// Declares end of current frame
			renderer.EndFrame();

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

	DEBUG_FUNC([&] {idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)); });

	return 0;
}