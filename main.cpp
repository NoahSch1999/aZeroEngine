#include "aZeroEditor.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

#include "pipeline/ScenePass.hpp"
#include "graphics_api/Wrappers/ResourceWrapping.hpp"
#include "graphics_api/Wrappers/VertexBuffer.hpp"
#include "graphics_api/Wrappers/CommandWrapping.hpp"
#include "misc/CallbackExecutor.hpp"

// Thanks to BetterComments2022 we can differentiate different type of comments! :)
//! This is an important comment
//x This is a crossed out comment
//? This is a question
//todo This is a todo comment

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

		std::shared_ptr<aZero::Window::RenderWindow> activeWindow = engine.CreateRenderWindow({ 1920, 1080 }, "aZero engine");
		Rendering::RenderContext renderContext = engine.GetRenderContext();
		IDxcCompilerX& compiler = engine.GetCompiler();

		std::shared_ptr<Pipeline::VertexShader> vs = std::make_shared<Pipeline::VertexShader>(Pipeline::VertexShader());
		vs->CompileFromFile(compiler, engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");

		std::shared_ptr<Pipeline::PixelShader> ps = std::make_shared<Pipeline::PixelShader>(Pipeline::PixelShader());
		ps->CompileFromFile(compiler, engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

		std::shared_ptr<Rendering::RenderSurface> SceneColorSurface(
			engine.CreateRenderSurface(activeWindow->GetClientDimensions(),
				Rendering::RenderSurface::Type::Color_Target, DXM::Vector4(0.f, 0.f, 0.f, 1.f)));

		std::shared_ptr<Rendering::RenderSurface> SceneDepthSurface(
			engine.CreateRenderSurface(activeWindow->GetClientDimensions(),
				Rendering::RenderSurface::Type::Depth_Target, DXM::Vector4(1, 0, 0, 0)));

		Pipeline::ScenePass scenePass;
		{
			Pipeline::ScenePass::PassDescription scenePassDesc;
			scenePassDesc.TopologyType = Pipeline::ScenePass::TOPOLOGY_TYPE::TRIANGLE;
			scenePassDesc.DepthStencil.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			Pipeline::ScenePass::PassDescription::RenderTarget rtv;
			rtv.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
			rtv.Name = "ColorTarget";
			scenePassDesc.RenderTargets.push_back(rtv);
			scenePass.Compile(engine.GetDevice(), scenePassDesc, vs, ps);

			renderContext.GetScenePass() = &scenePass;

			scenePass.BindDepthStencilTarget(SceneDepthSurface);

			scenePass.BindRenderTarget("ColorTarget", SceneColorSurface);
		}

		Pipeline::ComputeShader cs;
		cs.CompileFromFile(compiler, engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "TestShader.cs.hlsl");

		Asset::AssetManager& assetManager = engine.GetAssetManager();
		Asset::NewAssetManager& newAssetManager = engine.GetNewAssetManager();
		Scene::SceneManager& sceneManager = engine.GetSceneManager();
		//

		// Creating a scene
		auto sceneptr = sceneManager.CreateScene("MyScene").lock();
		Scene::Scene& scene = *sceneptr.get();

		// Creating a mesh
		auto myMesh = assetManager.CreateMesh("mesh");
		myMesh.GetAsset()->Load(engine.GetProjectDirectory() + MESH_ASSET_RELATIVE_PATH + "goblin.fbx");
		renderContext.UpdateRenderState(myMesh);

		Asset::NewMesh* newMesh = newAssetManager.CreateMesh("mesh");
		newMesh->Load(engine.GetProjectDirectory() + MESH_ASSET_RELATIVE_PATH + "goblin.fbx");
		renderContext.NewUpdateRenderState(newMesh);

		Asset::NewTexture* newAlbedo = newAssetManager.CreateTexture("texture");
		newAlbedo->Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinAlbedo.png",
			DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		renderContext.NewUpdateRenderState(newAlbedo);

		Asset::NewTexture* newNormalMap = newAssetManager.CreateTexture("normal");
		newNormalMap->Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinNormal.png",
			DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		renderContext.NewUpdateRenderState(newNormalMap);

		Asset::NewMaterial* newMaterial = newAssetManager.CreateMaterial("material");
		newMaterial->m_Data.AlbedoTexture = newAlbedo;
		newMaterial->m_Data.NormalMap = newNormalMap;
		renderContext.NewUpdateRenderState(newMaterial);

		// Creating a texture
		auto myTexture = assetManager.CreateTexture("texture");
		myTexture.GetAsset()->Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinAlbedo.png",
			DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		renderContext.UpdateRenderState(myTexture);

		auto myNormalMap = assetManager.CreateTexture("normal");
		myNormalMap.GetAsset()->Load(engine.GetProjectDirectory() + TEXTURE_ASSET_RELATIVE_PATH + "goblinNormal.png",
			DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		renderContext.UpdateRenderState(myNormalMap);

		// Creating a material
		auto myMaterial = assetManager.CreateMaterial("material");
		myMaterial.GetAsset()->m_Data.AlbedoTexture = myTexture;
		myMaterial.GetAsset()->m_Data.NormalMap = myNormalMap;
		renderContext.UpdateRenderState(myMaterial);

		PointLightData plData;
		{
			ECS::Entity ent = scene.CreateEntity();
			ECS::Entity ent2 = scene.CreateEntity();
			ECS::Entity ent3 = scene.CreateEntity();
			ECS::Entity ent4 = scene.CreateEntity();
			scene.RenameEntity(ent2, "noah");
			scene.RenameEntity(ent, "Entity_1");

			scene.GetComponentManager().GetComponent<ECS::TransformComponent>(ent2)->SetTransform(DXM::Matrix::CreateScale(0.5) * DXM::Matrix::CreateRotationY(3.1415) * DXM::Matrix::CreateTranslation(0, -1.5, 5));

			ECS::StaticMeshComponent CubeMeshComp;
			CubeMeshComp.m_MeshReference = myMesh;
			CubeMeshComp.m_MaterialReference = myMaterial;
			scene.GetComponentManager().AddComponent(ent2, CubeMeshComp);

			ECS::NewStaticMeshComponent NewCubeMeshComp;
			NewCubeMeshComp.m_MeshReference = newMesh;
			NewCubeMeshComp.m_MaterialReference = newMaterial;
			scene.GetComponentManager().AddComponent(ent2, NewCubeMeshComp);

			ECS::PointLightComponent pl;
			plData.Color = { 1,1,1 };
			plData.FalloffFactor = 1;
			plData.Intensity = 1;
			plData.Position = { 0,0,0 };
			pl.SetData(plData);
			scene.GetComponentManager().AddComponent(ent2, pl);

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

		float rot = 0.f;
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
			ECS::PointLightComponent& plComp = *scene.GetComponentManager().GetComponent<ECS::PointLightComponent>(camEnt);
			if (GetAsyncKeyState('W'))
			{
				cam.m_Position += DXM::Vector3(0, 0, 0.01f);
				plData.Position = { cam.m_Position };
				plComp.SetData(plData);
			}

			if (GetAsyncKeyState('S'))
			{
				cam.m_Position += DXM::Vector3(0, 0, -0.03f);
				plData.Position = { cam.m_Position };
				plComp.SetData(plData);
			}

			if (GetAsyncKeyState('D'))
			{
				cam.m_Position += DXM::Vector3(-0.01f, 0, 0);
				plData.Position = { cam.m_Position };
				plComp.SetData(plData);
			}

			if (GetAsyncKeyState('A'))
			{
				cam.m_Position += DXM::Vector3(0.01f, 0, 0);
				plData.Position = { cam.m_Position };
				plComp.SetData(plData);
			}
			//

			if (GetAsyncKeyState(VK_OEM_PLUS) || GetAsyncKeyState(VK_ADD))
			{
				plData.Intensity += 0.01;
				plComp.SetData(plData);
			}
			else if (GetAsyncKeyState(VK_OEM_MINUS) || GetAsyncKeyState(VK_SUBTRACT))
			{
				plData.Intensity -= 0.01;
				plComp.SetData(plData);
			}

			if (GetAsyncKeyState('1'))
			{
				if (!myMaterial.GetAsset()->m_Data.NormalMap.IsValid())
				{
					myMaterial.GetAsset()->m_Data.NormalMap = myNormalMap;
					renderContext.UpdateRenderState(myMaterial);
				}
			}
			else if (GetAsyncKeyState('2'))
			{
				myMaterial.GetAsset()->m_Data.NormalMap = Asset::AssetHandle<Asset::Texture>();
				renderContext.UpdateRenderState(myMaterial);
			}

			ECS::TransformComponent& tf = *scene.GetComponentManager().GetComponent<ECS::TransformComponent>(camEnt);
			tf.SetTransform(DXM::Matrix::CreateScale(0.5) * DXM::Matrix::CreateRotationY(3.1415 + rot) * DXM::Matrix::CreateTranslation(0, -1.3, 5));
			scene.UpdateRenderState(camEnt);

			//rot += 0.001f;

			// Example of render pipeline hotreloading
			if (GetAsyncKeyState(VK_SPACE))
			{
				vs->CompileFromFile(compiler, engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");
				ps->CompileFromFile(compiler, engine.GetProjectDirectory() + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

				Pipeline::ScenePass::PassDescription scenePassDesc;
				scenePassDesc.TopologyType = Pipeline::ScenePass::TOPOLOGY_TYPE::TRIANGLE;
				scenePassDesc.DepthStencil.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				Pipeline::ScenePass::PassDescription::RenderTarget rtv;
				rtv.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
				rtv.Name = "ColorTarget";
				scenePassDesc.RenderTargets.push_back(rtv);
				
				if (scenePass.Compile(engine.GetDevice(), scenePassDesc, vs, ps))
				{
					renderContext.GetScenePass() = &scenePass;
					scenePass.BindDepthStencilTarget(SceneDepthSurface);
					scenePass.BindRenderTarget("ColorTarget", SceneColorSurface);
				}
			}

			if (activeWindow->WaitOnSwapchain())
			{
				renderContext.Render(scene, *SceneColorSurface.get(), true, *SceneDepthSurface.get(), true);

				renderContext.CompleteRender(*SceneColorSurface.get(), activeWindow);

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