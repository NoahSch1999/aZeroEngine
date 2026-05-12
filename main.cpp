#include "aZeroEngine/Engine.hpp"

#include "src/RenderWindow.hpp"
#include "src/apiExamples.hpp"
#include "src/EditorGUI.hpp"
#include "assets/AssetManager.hpp"

#ifdef RUN_TESTS
#include "tests/Tests.hpp"
#endif

#if USE_DEBUG
#include <dxgidebug.h>
#endif

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

#include "renderer/SceneRenderData_NEW.hpp"
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
		Asset::Mesh mesh;
		mesh.LoadFromFile(Asset::GetMeshDirectoryPath() + "cube2.fbx");
		aZero::Engine engine(3);
		Rendering::Renderer& renderer = engine.GetRenderer();
		Rendering::WireframeRenderer& wireframeRenderer = renderer.GetWireframeRenderer();
		Asset::AssetManager& assetManager = engine.GetAssetManager();
		Audio::AudioEngine& audioEngine = engine.GetAudioEngine();
		Physics::PhysicsEngine& pEngine = engine.GetPhysicsEngine();
		//

#ifdef RUN_TESTS
		RunTests(engine);
#endif

		// Create your own implemented window and swapchain + input system
		//RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,1200,800/*2560,1440*/ }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		//RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,2560,1440 }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		RenderWindow window(Window::WindowDesc("MyWindow", { 0,0,1920,1080 }, { 1,1,0,1 }, SDL_WINDOW_RESIZABLE), renderer);
		//

		// Create render surfaces
		auto [width, height] = window.GetClientDimensions();
		auto rtv = renderer.CreateRenderTarget(Rendering::RenderTarget::Desc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, { 0.3,0.3,0.3,1 }, true));
		auto dsv = renderer.CreateDepthStencilTarget(Rendering::DepthStencilTarget::Desc(width, height, 1, 0, true, true));

		aZero::ImGui_Wrapper::Init(renderer, window.GetSDLWindow());

		Editor::GUI::EditorGUI editorGUI(window.GetDeviceManager(), renderer.GetWireframeRenderer(), engine.GetAssetManager());

		Input::KeyboardListener keyboardListener;
		Scene::Scene scene(engine.GetPhysicsEngine());
		Example::Setup(engine, scene, { (float)width, (float)height }, rtv, dsv, window, keyboardListener);

		assetManager.RegisterScene(scene);

		renderer.FlushFrameAllocations();

		int frame = 0;
		while (window.IsOpen())
		{
			window.Update();
			scene.UpdateTemp();

			// Declares start of new frame and loops until the new frame can be rendered
			while (!renderer.TryBeginFrame())
			{
				// Do some stuff while waiting, ex. queue physics calcs on a seperate thread
			}

			aZero::ImGui_Wrapper::BeginFrame();

			if (frame % 3 == 2)
			{
				scene.UpdatePhysics(true);
			}

			editorGUI.Update(scene);

			Example::ControlCamera(scene, keyboardListener);

			renderer.Render(scene, rtv, dsv);

			flecs::entity camEnt = scene.GetEntityWorld().lookup("Camera");
			wireframeRenderer.Render(camEnt.get<Component::Camera>(), camEnt.get<Component::Position>(), camEnt.get<Component::Rotation>(), rtv, dsv);

			editorGUI.Render(renderer, rtv);
			
			renderer.CopyRenderTargetToSwapChain(window.GetSwapChain(), rtv);

			renderer.EndFrame();

			window.Present();
			frame++;
		}

		renderer.FlushRenderCommands();

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
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