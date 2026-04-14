#include "aZeroEditor.hpp"
#include "RenderWindow.hpp"

#include "apiExamples.hpp"

#ifdef RUN_TESTS
#include "tests/Tests.hpp"
#endif

//#include "audio/AudioSystem.hpp"

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

		auto m = Asset::LoadFromFile("cube.fbx");

#ifdef RUN_TESTS
		RunTests(engine);
#endif

		// Create your own implemented window and swapchain + input system
		RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,800,600 }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		//

		// Create render surfaces
		auto [width, height] = window.GetClientDimensions();
		auto [rtv, dsv] = CreateRenderSurfaces(engine, { (float)width, (float)height });
		//
		
		// Scene setup
		// TODO: Seperate the asset classes from file and name related stuff.
		//		That way its easier to handle both file-assets and programmatically created assets in the same way.
		//		An asset doesn't need a "name" unless it needs to either be fetched from some cache OR related to a filepath.
		Asset::Mesh mesh(0);
		Asset::Material material(0);
		Asset::Texture albedo(0);
		Asset::Texture normalMap(0);
		LoadAssets(engine, mesh, material, albedo, normalMap);

		Scene::SceneNew scene;
		CreateScene(scene, mesh, material, { (float)width, (float)height });
		//

		float rot = 0.f;

		Scene::SceneNew::ComponentFlag flag;
		int32_t fl = 0;
		fl |= (int32_t)Scene::SceneNew::ComponentFlag::Transform;
		fl |= (int32_t)Scene::SceneNew::ComponentFlag::Camera;

		Input::KeyboardListener listener = window.GetDeviceManager().ListenKeyboard({
			[&window](const SDL_Event& event, Input::Keyboard& keyboard) {
				if (event.key.key == SDLK_RETURN)
					window.Close();
			},
			[](const SDL_Event& event, Input::Keyboard& keyboard) { }
			});

		std::cout << "Render-loop started!\n";
		while (window.IsOpen())
		{
			window.Update();
			//continue;
			// Call "NewFrame()" with API
			while (!renderer.BeginFrame())
			{
				// Do some stuff while waiting
			}

			// Example of moving the camera
			ECS::Entity camEnt = scene.GetEntity("CameraEntity").value();
			ECS::CameraComponent& cam = *scene.m_ComponentManager.GetComponent<ECS::CameraComponent>(camEnt);
			if (GetAsyncKeyState('W'))
			{
				cam.m_Position += DXM::Vector3(0, 0, 0.01f);
			}

			if (GetAsyncKeyState('S'))
			{
				cam.m_Position += DXM::Vector3(0, 0, -0.03f);
			}

			if (GetAsyncKeyState('D'))
			{
				cam.m_Position += DXM::Vector3(-0.01f, 0, 0);
			}

			if (GetAsyncKeyState('A'))
			{
				cam.m_Position += DXM::Vector3(0.01f, 0, 0);
			}

			ECS::TransformComponent& tf = *scene.m_ComponentManager.GetComponent<ECS::TransformComponent>(camEnt);
			tf.SetTransform(DXM::Matrix::CreateScale(0.5) * DXM::Matrix::CreateRotationY(3.1415 + rot) * DXM::Matrix::CreateTranslation(0, -1.3, 5));
			scene.MarkRenderStateDirty(camEnt, aZero::Scene::SceneNew::ComponentFlag());
			//

			// Call "Render()" with the API
			renderer.Render(scene, &rtv, &dsv);

			// Call the helper function in the API that copies a rendersurface to a swapchain backbuffer
			renderer.EndFrame();

			renderer.CopyTextureToTexture(window.GetCurrentBackbuffer(), rtv.GetTexture().GetResource());

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

	// Link error...
	//DEBUG_FUNC([&] {idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)); });

	return 0;
}