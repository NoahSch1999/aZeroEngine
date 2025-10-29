#include "aZeroEditor.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;
using namespace Rendering;

#include "assets/AssetManager.hpp"
#include "scene/SceneManager.hpp"

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

		aZero::AssetNew::AssetManager am;
		auto myMesh = am.CreateMesh("mesh");
		auto mys = am.CreateMesh("meshsa");
		auto x = am.CreateMesh("meshs");
		auto myMaterial = am.CreateMaterial("material");
		auto myTexture = am.CreateTexture("texture");

		SceneTemp::SceneManager sm;
		RendererNew rendNew;
		rendNew.Init(Engine.GetDevice(), 3, aZero::Helper::GetProjectDirectory() + "/../../../content");
		AssetNew::Mesh& mesh = *myMesh.lock();
		mesh.Load("C:/Projects/Programming/aZeroEditor/content/assets/meshes/cube.fbx");
		rendNew.Upload(mesh);

		AssetNew::Mesh& meshs = *mys.lock();
		meshs.Load("C:/Projects/Programming/aZeroEditor/content/assets/meshes/cube.fbx");
		rendNew.Upload(mesh);

		myMaterial.lock()->m_Data.AlbedoTexture = myTexture;
		myMaterial.lock()->m_Data.NormalMap = myTexture;

		myTexture.lock()->Load("C:/Users/Noah Schierenbeck/Pictures/Funny Pictures/freakycat.png");
		rendNew.Upload(*myTexture.lock());

		auto cmdList = rendNew.m_CommandContextAllocator.GetContext()->m_Context->GetCommandList();
		rendNew.m_FrameAllocator.RecordAllocations(cmdList);
		rendNew.m_GraphicsQueue.ExecuteContext(*rendNew.m_CommandContextAllocator.GetContext()->m_Context);

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

		auto sceneptr = sm.CreateScene("MyScene").lock();
		Scene::SceneNew& scene = *sceneptr.get();
		{
			ECS::Entity ent = scene.CreateEntity();
			ECS::Entity ent2 = scene.CreateEntity();
			ECS::Entity ent3 = scene.CreateEntity();
			ECS::Entity ent4 = scene.CreateEntity();
			scene.RenameEntity(ent2, "noah");
			scene.RenameEntity(ent, "Entity_1");

			scene.GetComponentManager().GetComponent<ECS::TransformComponent>(ent2)->SetTransform(DXM::Matrix::CreateScale(2) /** DXM::Matrix::CreateRotationY(3.1415 / 2)*/ * DXM::Matrix::CreateTranslation(1337, 5, 42));

			ECS::StaticMeshComponent CubeMeshComp;
			CubeMeshComp.m_MeshReference = Engine.GetCubeMesh();
			CubeMeshComp.m_MaterialReference = Engine.GetDefaultMaterial();
			scene.GetComponentManager().AddComponent(ent2, CubeMeshComp);

			ECS::CameraComponent CamComp;
			CamComp.m_TopLeft = { 0,0 };
			CamComp.m_Dimensions = ActiveWindow->GetClientDimensions();
			CamComp.m_NearPlane = 0.001f;
			CamComp.m_FarPlane = 1000.f;
			CamComp.m_Fov = 3.14 / 2.f;
			scene.GetComponentManager().AddComponent(ent2, CamComp);

			scene.MarkRenderStateDirty(ent2);

			Scene::SceneSerializer::Serialize(scene, "C:/Projects/Programming/aZeroEditor/idk.aZene");
			auto loadedScene = Scene::SceneSerializer::Deserialize("C:/Projects/Programming/aZeroEditor/idk.aZene");
		}
		
		//Engine.m_RendererNew.Render(scene, SceneColorSurface.GetView<D3D12::RenderTargetView>(), true, SceneDepthSurface.GetView<D3D12::DepthStencilView>(), true);

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