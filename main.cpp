#include "aZeroEditor.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

#include "pipeline/ScenePass.hpp"

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCommand)
{
#if USE_DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	Microsoft::WRL::ComPtr<ID3D12Debug> d3d12Debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
		d3d12Debug->EnableDebugLayer();

	CComPtr<ID3D12Debug> spDebugController0;
	CComPtr<ID3D12Debug1> spDebugController1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0));
	spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1));
	spDebugController1->SetEnableGPUBasedValidation(true);

	Microsoft::WRL::ComPtr<IDXGIDebug> idxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&idxgiDebug));
#endif // DEBUG

	try
	{
		// API Interfaces
		aZero::Engine engine(3, aZero::Helper::GetProjectDirectory() + "/../../../content");

		Pipeline::VertexShader vs;
		vs.CompileFromFile(engine.GetPipelineManager().GetCompiler(), engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");

		Pipeline::PixelShader ps;
		ps.CompileFromFile(engine.GetPipelineManager().GetCompiler(), engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

		Pipeline::ComputeShader cs;
		cs.CompileFromFile(engine.GetPipelineManager().GetCompiler(), engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "TestShader.cs.hlsl");

		std::shared_ptr<aZero::Window::RenderWindow> activeWindow = engine.CreateRenderWindow({ 1920, 1080 }, "aZero engine");
		Rendering::RenderContext renderContext = engine.GetRenderContext();
		Asset::AssetManager& assetManager = engine.GetAssetManager();
		Scene::SceneManager& sceneManager = engine.GetSceneManager();
		//

		// Creating a scene
		auto sceneptr = sceneManager.CreateScene("MyScene").lock();
		Scene::Scene& scene = *sceneptr.get();

		// Creating a mesh
		auto myMesh = assetManager.CreateMesh("mesh");
		myMesh.GetAsset()->Load(engine.GetProjectDirectory() + MESH_ASSET_RELATIVE_PATH + "unitCube.fbx");
		renderContext.UpdateRenderState(myMesh);

		// Creating a texture
		auto myTexture = assetManager.CreateTexture("texture");
		myTexture.GetAsset()->Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "freakycat.png");
		renderContext.UpdateRenderState(myTexture);

		// Creating a material
		auto myMaterial = assetManager.CreateMaterial("material");
		myMaterial.GetAsset()->m_Data.AlbedoTexture = myTexture;
		myMaterial.GetAsset()->m_Data.NormalMap = myTexture;
		renderContext.UpdateRenderState(myMaterial);


		{
			ECS::Entity ent = scene.CreateEntity();
			ECS::Entity ent2 = scene.CreateEntity();
			ECS::Entity ent3 = scene.CreateEntity();
			ECS::Entity ent4 = scene.CreateEntity();
			scene.RenameEntity(ent2, "noah");
			scene.RenameEntity(ent, "Entity_1");

			scene.GetComponentManager().GetComponent<ECS::TransformComponent>(ent2)->SetTransform(DXM::Matrix::CreateScale(2) /** DXM::Matrix::CreateRotationY(3.1415 / 2)*/ * DXM::Matrix::CreateTranslation(0, 0, 42));

			ECS::StaticMeshComponent CubeMeshComp;
			CubeMeshComp.m_MeshReference = myMesh;
			CubeMeshComp.m_MaterialReference = myMaterial;
			scene.GetComponentManager().AddComponent(ent2, CubeMeshComp);

			ECS::CameraComponent cameraComp;
			cameraComp.m_TopLeft = { 0,0 };
			cameraComp.m_Dimensions = activeWindow->GetClientDimensions();
			cameraComp.m_NearPlane = 0.001f;
			cameraComp.m_FarPlane = 1000.f;
			cameraComp.m_Fov = 3.14 / 2.f;
			scene.GetComponentManager().AddComponent(ent2, cameraComp);

			scene.UpdateRenderState(ent2);

			//Scene::SceneSerializer::Serialize(scene, "C:/Projects/PrograssetManagerming/aZeroEditor/idk.aZene");
			//auto loadedScene = Scene::SceneSerializer::Deserialize("C:/Projects/PrograssetManagerming/aZeroEditor/idk.aZene");
		}

		Rendering::RenderSurface SceneColorSurface(
			engine.CreateRenderSurface(activeWindow->GetClientDimensions(), 
				Rendering::RenderSurface::Type::Color_Target, DXM::Vector4(0.2,0.2,0.2,0)));
		
		Rendering::RenderSurface SceneDepthSurface(
			engine.CreateRenderSurface(activeWindow->GetClientDimensions(), 
				Rendering::RenderSurface::Type::Depth_Target));

		while (activeWindow->IsOpen())
		{
			activeWindow->HandleMessages();

			renderContext.BeginFrame();

			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			// Example of moving the camera
			ECS::Entity camEnt = scene.GetEntity("noah").value();
			ECS::CameraComponent& cam = *scene.GetComponentManager().GetComponent<ECS::CameraComponent>(camEnt);
			if (GetAsyncKeyState('W'))
			{
				cam.m_Position += DXM::Vector3(0, 0, 0.01f);
				scene.UpdateRenderState(camEnt);
			}

			if (GetAsyncKeyState('S'))
			{
				cam.m_Position += DXM::Vector3(0, 0, -0.03f);
				scene.UpdateRenderState(camEnt);
			}

			if (GetAsyncKeyState('D'))
			{
				cam.m_Position += DXM::Vector3(-0.01f, 0, 0);
				scene.UpdateRenderState(camEnt);
			}

			if (GetAsyncKeyState('A'))
			{
				cam.m_Position += DXM::Vector3(0.01f, 0, 0);
				scene.UpdateRenderState(camEnt);
			}
			//

			// Example of render pipeline hotreloading
			if (GetAsyncKeyState(VK_SPACE))
			{
				renderContext.HotreloadRenderPipeline();
			}

			if (activeWindow->WaitOnSwapchain())
			{
				renderContext.Render(scene, SceneColorSurface, true, SceneDepthSurface, true);

				renderContext.CompleteRender(SceneColorSurface, activeWindow);

				renderContext.EndFrame();

				activeWindow->Present();
			}
		}

		renderContext.FlushRenderingCommands();
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