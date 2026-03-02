#include "aZeroEditor.hpp"

#include "windowCreation.hpp"
#include "apiExamples.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCommand)
{
#if USE_DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	// CPU-side validation layer
	Microsoft::WRL::ComPtr<ID3D12Debug> d3d12Debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
		d3d12Debug->EnableDebugLayer();

	// GPU-side validation layer
	CComPtr<ID3D12Debug> dbContr0;
	CComPtr<ID3D12Debug1> dbContr1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&dbContr0));
	dbContr0->QueryInterface(IID_PPV_ARGS(&dbContr1));
	dbContr1->SetEnableGPUBasedValidation(true);

	Microsoft::WRL::ComPtr<IDXGIDebug> idxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&idxgiDebug));
#endif // DEBUG

	try
	{
		// API Interfaces
		aZero::Engine engine(3, aZero::Helper::GetProjectDirectory() + "/../../../content");
		Rendering::Renderer& renderer = engine.GetRenderer();
		//

		// Create your own implemented window and swapchain
		WindowWrapper window(renderer, "Window", { 1920, 1080 }, { 1920, 1080 });
		//

		// Create render surfaces
		auto [rtv, dsv] = CreateRenderSurfaces(engine, { window.GetClientDimensions() });
		//

		// Scene setup
		Asset::Mesh mesh("mesh");
		Asset::Material material("material");
		Asset::Texture albedo("albedo");
		Asset::Texture normalMap("normalMap");
		LoadAssets(engine, mesh, material, albedo, normalMap);

		Scene::Scene scene(0, "MyScene");
		CreateScene(scene, mesh, material, window.GetClientDimensions());
		//

		float rot = 0.f;

		std::cout << "Render-loop started!\n";
		while (window.IsOpen())
		{
			window.HandleMessages();

			// Call "NewFrame()" with API
			renderer.BeginFrame();

			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			// Example of moving the camera
			ECS::Entity camEnt = scene.GetEntity("CameraEntity").value();
			ECS::CameraComponent& cam = *scene.GetComponentManager().GetComponent<ECS::CameraComponent>(camEnt);
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

			ECS::TransformComponent& tf = *scene.GetComponentManager().GetComponent<ECS::TransformComponent>(camEnt);
			tf.SetTransform(DXM::Matrix::CreateScale(0.5) * DXM::Matrix::CreateRotationY(3.1415 + rot) * DXM::Matrix::CreateTranslation(0, -1.3, 5));
			scene.UpdateRenderState(camEnt);
			//

			// Call "Render()" with the API
			renderer.Render(scene, &rtv, &dsv);

			// Call the helper function in the API that copies a rendersurface to a swapchain backbuffer

			window.Present();
		}

		renderer.FlushGPU();
	}
	catch (std::invalid_argument& e)
	{
		printf(e.what());
		DebugBreak();
	}

	// Link error...
	//DEBUG_FUNC([&] {idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)); });

	return 0;
}