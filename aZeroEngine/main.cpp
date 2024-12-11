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
		
		Asset::Mesh Torus;
		Torus.LoadFromFile(ASSET_PATH + "meshes/Torus.fbx");
		RenderInterface.MarkRenderStateDirty(Torus);

	/*	{*/
			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = Torus;
			MeshComp.m_MaterialReference = Engine.Material;
			CManager.AddComponent(Ent2.GetEntity(), MeshComp);
	/*	}*/

			RenderInterface.GetEntityToMesh()[Ent2.GetEntity().GetID()] = Torus;
			RenderInterface.MarkRenderStateDirty<ECS::StaticMeshComponent>(Ent2);
		//
		
		while (ActiveWindow->IsOpen())
		{
			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}
			else if (GetAsyncKeyState('2'))
			{
				ECS::StaticMeshComponent* Comp = CManager.GetComponent<ECS::StaticMeshComponent>(Ent2.GetEntity());
				Comp->m_MeshReference = Engine.Mesh;
				RenderInterface.MarkRenderStateDirty<ECS::StaticMeshComponent>(Ent2);
				RenderInterface.GetEntityToMesh()[Ent2.GetEntity().GetID()] = Engine.Mesh;
			}
			else if (GetAsyncKeyState('3'))
			{
				ECS::StaticMeshComponent* Comp = CManager.GetComponent<ECS::StaticMeshComponent>(Ent2.GetEntity());
				Comp->m_MeshReference = Torus;
				RenderInterface.MarkRenderStateDirty<ECS::StaticMeshComponent>(Ent2);
				RenderInterface.GetEntityToMesh()[Ent2.GetEntity().GetID()] = Torus;
			}

			Engine.BeginFrame();

			// Example
			static float Rot = 0.f;
			Rot += 0.005f;
			CManager.GetComponent<ECS::TransformComponent>(Ent2.GetEntity())->SetTransform(DXM::Matrix::CreateScale(0.5f) * DXM::Matrix::CreateRotationY(Rot) * DXM::Matrix::CreateTranslation(DXM::Vector3(0,0,10.f)));
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