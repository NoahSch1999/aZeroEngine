#include "aZeroEditor.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;
using namespace Rendering;

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCommand)
{
#if USE_DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	Microsoft::WRL::ComPtr<ID3D12Debug> d3d12Debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
		d3d12Debug->EnableDebugLayer();

	Microsoft::WRL::ComPtr<IDXGIDebug> idxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&idxgiDebug));
#endif // DEBUG

	try
	{
		aZero::Engine Engine(3, aZero::Helper::GetProjectDirectory() + "/../../../content");
		Rendering::RenderContext RenderContext = Engine.GetRenderContext();

		std::shared_ptr<aZero::Window::RenderWindow> ActiveWindow = Engine.CreateRenderWindow({ 1920, 1080 }, "aZero Engine");

		Rendering::RenderSurface SceneColorSurface(
			Engine.CreateRenderSurface(ActiveWindow->GetClientDimensions(), 
				Rendering::RenderSurface::Type::Color_Target, DXM::Vector4(0.2,0.2,0.2,0)));
		
		Rendering::RenderSurface SceneDepthSurface(
			Engine.CreateRenderSurface(ActiveWindow->GetClientDimensions(), 
				Rendering::RenderSurface::Type::Depth_Target));

		// Scene creation
		Scene::Scene CurrentScene = Engine.CreateScene();
		{
			Scene::SceneEntity& CubeEntity = *CurrentScene.CreateEntity("Cube");
			ECS::TransformComponent CubeTf;
			CubeTf.SetTransform(DXM::Matrix::CreateTranslation(0, 0, 3));
			CurrentScene.AddComponent<ECS::TransformComponent>(CubeEntity, std::move(CubeTf));
			ECS::StaticMeshComponent CubeMeshComp;
			CubeMeshComp.m_MeshReference = Engine.GetCubeMesh();
			CubeMeshComp.m_MaterialReference = Engine.GetDefaultMaterial();
			CurrentScene.AddComponent<ECS::StaticMeshComponent>(CubeEntity, std::move(CubeMeshComp));
			CurrentScene.MarkRenderStateDirty(CubeEntity);

			Scene::SceneEntity& CameraEntity = *CurrentScene.CreateEntity("Camera");
			ECS::CameraComponent CamComp;
			CamComp.m_TopLeft = { 0,0 };
			CamComp.m_Dimensions = ActiveWindow->GetClientDimensions();
			CamComp.m_NearPlane = 0.001f;
			CamComp.m_FarPlane = 1000.f;
			CamComp.m_Fov = 3.14 / 2.f;
			CurrentScene.AddComponent(CameraEntity, std::move(CamComp));
			CurrentScene.MarkRenderStateDirty(CameraEntity);
		}
		//

		RenderContext.FlushRenderingCommands();

		while (ActiveWindow->IsOpen())
		{
			ActiveWindow->HandleMessages();

			RenderContext.BeginRenderFrame();

			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			Scene::SceneEntity& CameraEntity = *CurrentScene.GetEntity("Camera");
			if (GetAsyncKeyState('W'))
			{
				ECS::CameraComponent* Cam = CurrentScene.GetComponent<ECS::CameraComponent>(CameraEntity);
				Cam->m_Position += DXM::Vector3(0, 0, 0.01f);
				CurrentScene.MarkRenderStateDirty(CameraEntity);
			}

			if (GetAsyncKeyState('S'))
			{
				ECS::CameraComponent* Cam = CurrentScene.GetComponent<ECS::CameraComponent>(CameraEntity);
				Cam->m_Position += DXM::Vector3(0, 0, -0.03f);
				CurrentScene.MarkRenderStateDirty(CameraEntity);
			}

			if (GetAsyncKeyState('D'))
			{
				ECS::CameraComponent* Cam = CurrentScene.GetComponent<ECS::CameraComponent>(CameraEntity);
				Cam->m_Position += DXM::Vector3(-0.01f, 0, 0);
				CurrentScene.MarkRenderStateDirty(CameraEntity);
			}

			if (GetAsyncKeyState('A'))
			{
				ECS::CameraComponent* Cam = CurrentScene.GetComponent<ECS::CameraComponent>(CameraEntity);
				Cam->m_Position += DXM::Vector3(0.01f, 0, 0);
				CurrentScene.MarkRenderStateDirty(CameraEntity);
			}

			// Re-run frame if swapchain isnt ready for a new frame
			// This reduces input latency (in theory) 
			if (ActiveWindow->WaitOnSwapchain())
			{
				RenderContext.Render(CurrentScene, SceneColorSurface, true, SceneDepthSurface, true);

				RenderContext.CompleteRender(SceneColorSurface, ActiveWindow);

				RenderContext.EndRenderFrame();

				ActiveWindow->Present();
			}
		}

		RenderContext.FlushRenderingCommands();
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