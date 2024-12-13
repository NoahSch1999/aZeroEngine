#include "Core/Engine.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCommand)
{
#ifdef _DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	Microsoft::WRL::ComPtr<ID3D12Debug> spDebugController0;
	Microsoft::WRL::ComPtr<ID3D12Debug1> spDebugController1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0));

	Microsoft::WRL::ComPtr<ID3D12Debug> d3d12Debug;
	Microsoft::WRL::ComPtr<IDXGIDebug> idxgiDebug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
		d3d12Debug->EnableDebugLayer();
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&idxgiDebug));
#endif // DEBUG

	try
	{
		// Setup interfaces
		aZero::Engine Engine(instance, { 1920, 1080 });
		std::shared_ptr<aZero::Window> ActiveWindow = Engine.GetWindow();
		ECS::ComponentManagerDecl& CManager = Engine.GetComponentManager();
		Rendering::RenderInterface RenderInterface = Engine.CreateRenderInterface();
		//

		// Example
		aZero::Scene* TestScene = Engine.CreateScene("Test");
		Engine.SetScene(TestScene);

		aZero::SceneEntity& Ent2 = TestScene->CreateEntity("P2");
		CManager.AddComponent(Ent2.GetEntity(), ECS::TransformComponent(DXM::Matrix::CreateScale(0.5) * DXM::Matrix::CreateTranslation(DXM::Vector3(0, 2, 10.f))));
		
		std::shared_ptr<Asset::Mesh> GoblinMesh = std::make_shared<Asset::Mesh>();
		GoblinMesh->LoadFromFile(ASSET_PATH + "meshes/goblin.fbx");
		RenderInterface.MarkRenderStateDirty(*GoblinMesh.get());

		std::shared_ptr<Asset::Texture> GoblinAlbedo = std::make_shared<Asset::Texture>();
		GoblinAlbedo->LoadFromFile(ASSET_PATH + "textures/goblinAlbedo.png");
		RenderInterface.MarkRenderStateDirty(*GoblinAlbedo.get());

		std::shared_ptr<Asset::Texture> GoblinNormal = std::make_shared<Asset::Texture>();
		GoblinNormal->LoadFromFile(ASSET_PATH + "textures/goblinNormal.png");
		RenderInterface.MarkRenderStateDirty(*GoblinNormal.get()/*, DXGI_FORMAT_R16G16B16A16_FLOAT*/);

		std::shared_ptr<Asset::Material> GoblinMat = std::make_shared<Asset::Material>();
		GoblinMat->SetAlbedoTexture(GoblinAlbedo);
		GoblinMat->SetNormalMap(GoblinNormal);
		RenderInterface.MarkRenderStateDirty(*GoblinMat.get());

		{
			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = GoblinMesh;
			MeshComp.m_MaterialReference = GoblinMat;
			CManager.AddComponent(Ent2.GetEntity(), MeshComp);
			RenderInterface.MarkRenderStateDirty<ECS::StaticMeshComponent>(Ent2);
		}
		//
		
		while (ActiveWindow->IsOpen())
		{
			Engine.BeginFrame();

			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			// Example
			static float Rot = 90.f;

			if (GetAsyncKeyState('1'))
			{
				Rot = 90.f;
			}
			else if (GetAsyncKeyState('2'))
			{
				

			}


			Rot += 0.0002f;
			CManager.GetComponent<ECS::TransformComponent>(Ent2.GetEntity())->SetTransform(DXM::Matrix::CreateScale(0.5f) * DXM::Matrix::CreateRotationY(Rot) * DXM::Matrix::CreateTranslation(DXM::Vector3(0,-1,3.f)));
			RenderInterface.MarkRenderStateDirty<ECS::TransformComponent>(Ent2);
			//

			Engine.Update();

			Engine.Render();
		}
	}
	catch(std::invalid_argument& e)
	{
		printf(e.what());
		DebugBreak();
	}

	DEBUG_FUNC([&] {idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)); });

	return 0;
}