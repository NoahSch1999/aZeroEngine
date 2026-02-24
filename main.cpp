#include "aZeroEditor.hpp"

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

		Asset::AssetManager& newAssetManager = engine.GetNewAssetManager();
		Rendering::Renderer& renderContext = engine.GetRenderer();
		//

		float rot = 0.f;

		std::cout << "Render-loop started!\n";
		while (/*activeWindow->IsOpen()*/true)
		{
			//activeWindow->HandleMessages();

			//renderContext.BeginFrame();

			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			// Example of moving the camera
			/*ECS::Entity camEnt = scene.GetEntity("Camera").value();
			ECS::CameraComponent& cam = *scene.GetComponentManager().GetComponent<ECS::CameraComponent>(camEnt);
			ECS::PointLightComponent& plComp = *scene.GetComponentManager().GetComponent<ECS::PointLightComponent>(camEnt);
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
			scene.UpdateRenderState(camEnt);*/
			//

			if (/*activeWindow->WaitOnSwapchain()*/true)
			{
				/*renderContext.Render(scene, *SceneColorSurface.get(), true, *SceneDepthSurface.get(), true);

				renderContext.CompleteRender(*SceneColorSurface.get(), activeWindow);*/

				/*renderContext.EndFrame();

				activeWindow->Present();*/
			}
		}

		renderContext.FlushGPU();
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