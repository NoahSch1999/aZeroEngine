#include "Core/Engine.h"

#include "Core/Renderer/RenderAssetManager.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

/*
TODO-LIST:
!. Cleanup new scene rendering code
	Includes culling, function args, Camera and more...

!. Get rid of mesh and material pointers and make the render culling just use local primitive array data

!. Add point, spot and directional phong lighting
	All should be scene primitives and work with the culling system

!. Add frustrum culling for relevant lights and primitives for each camera rendering

!. Remove the use of "primitive" in names. rename to something else
!. Add animation component etc
	Includes skeleton asset, animation asset, weights, specific animation pass, skeletalmeshcomponent(?)
	Keep in mind to design so both frustrum and octree culling can be used with it. (maybe generate some max bounds based on animations added?)

!. Add movable/static flag to scene entities which allow seperate paths for rendering and culling

!. Add octree to scene which only is used for entities with the static flags enabled

!. Change folder structure so it follows a more professional structure (ex. src, include, etc...)

!. Make engine a static library

!. Rework engine path system (ex. ASSET_PATH)
	Maybe make a library user define the paths as pre-processors or make then call ex. Engine::SetPaths etc...
	Some paths, for ex. shaders or default assets might be fixed relative to the .lib file
*/
int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCommand)
{
#ifdef _DEBUG
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
		aZero::Engine Engine(instance, { 1920, 1080 }, 3);
		std::shared_ptr<aZero::Window::RenderWindow> ActiveWindow = Engine.CreateRenderWindow({ 1920, 1080 }, "aZero Engine");
		std::shared_ptr<aZero::Window::RenderWindow> x = Engine.CreateRenderWindow({ 800, 800 }, "idk");

		Rendering::RenderInterface RenderInterface = Engine.CreateRenderInterface();

		Asset::RenderAssetManager& AssetMan = RenderInterface.GetAssetManager();

		NewScene::Scene idk = Engine.CreateScene();
		NewScene::SceneEntity& EntIDK = *idk.CreateEntity("hej");
		NewScene::SceneEntity& EntIDK3 = *idk.CreateEntity("hej");

		std::shared_ptr<Asset::Mesh> GoblinMesh = AssetMan.CreateAsset<Asset::Mesh>();
		GoblinMesh->LoadFromFile(ASSET_PATH + "meshes/goblin.fbx");
		RenderInterface.MarkRenderStateDirty(GoblinMesh);

		std::shared_ptr<Asset::Texture> GoblinAlbedo = AssetMan.CreateAsset<Asset::Texture>();
		GoblinAlbedo->LoadFromFile(ASSET_PATH + "textures/goblinAlbedo.png");
		RenderInterface.MarkRenderStateDirty(GoblinAlbedo);

		std::shared_ptr<Asset::Texture> GoblinNorm = AssetMan.CreateAsset<Asset::Texture>();
		GoblinNorm->LoadFromFile(ASSET_PATH + "textures/goblinNormal.png");
		RenderInterface.MarkRenderStateDirty(GoblinNorm);

		std::shared_ptr<Asset::Material> GoblinMat = AssetMan.CreateAsset<Asset::Material>();
		Asset::MaterialData MatData;
		MatData.m_AlbedoTexture = GoblinAlbedo;
		MatData.m_NormalMap = GoblinNorm;
		GoblinMat->SetData(std::move(MatData));
		RenderInterface.MarkRenderStateDirty(GoblinMat);

		{
			ECS::TransformComponent idktf;
			idktf.SetTransform(DXM::Matrix::CreateTranslation(0, 0, 3));
			idk.AddComponent<ECS::TransformComponent>(EntIDK, std::move(idktf));

			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = GoblinMesh;
			MeshComp.m_MaterialReference = GoblinMat;
			idk.AddComponent<ECS::StaticMeshComponent>(EntIDK, std::move(MeshComp));

			ECS::CameraComponent Cam;
			Cam.m_TopLeft = { 0,0 };
			Cam.m_Dimensions = { ActiveWindow->GetDimensions().x / 2.f, ActiveWindow->GetDimensions().y };
			//Cam.m_Dimensions = ActiveWindow->GetDimensions();
			Cam.m_NearPlane = 0.01f;
			Cam.m_FarPlane = 1000.f;
			Cam.m_Fov = 3.14 / 2.f;
			idk.AddComponent(EntIDK, std::move(Cam));

			idk.MarkRenderStateDirty(EntIDK);
		}

		{
			ECS::TransformComponent idktf;
			idktf.SetTransform(DXM::Matrix::CreateRotationY(90.f) * DXM::Matrix::CreateScale(0.2) * DXM::Matrix::CreateTranslation(1, 0.5, 3));
			idk.AddComponent<ECS::TransformComponent>(EntIDK3, std::move(idktf));

			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = GoblinMesh;
			MeshComp.m_MaterialReference = GoblinMat;
			idk.AddComponent<ECS::StaticMeshComponent>(EntIDK3, std::move(MeshComp));

			ECS::CameraComponent Cam;
			Cam.m_TopLeft = { ActiveWindow->GetDimensions().x / 2.f,0 };
			Cam.m_Dimensions = { ActiveWindow->GetDimensions().x / 2.f, ActiveWindow->GetDimensions().y };
			Cam.m_NearPlane = 0.01f;
			Cam.m_FarPlane = 1000.f;
			Cam.m_Fov = 3.14 / 2.f;
			idk.AddComponent(EntIDK3, std::move(Cam));

			idk.MarkRenderStateDirty(EntIDK3);

			NewScene::SceneEntity& EntOther = *idk.CreateEntity("hej");
			ECS::TransformComponent idktfs;
			idktfs.SetTransform(DXM::Matrix::CreateRotationY(180.f) * DXM::Matrix::CreateScale(0.2) * DXM::Matrix::CreateTranslation(1, 0, 2));
			idk.AddComponent<ECS::TransformComponent>(EntOther, std::move(idktfs));

			ECS::StaticMeshComponent MeshCompx;
			MeshCompx.m_MeshReference = Engine.GetCubeMesh();
			MeshCompx.m_MaterialReference = Engine.GetDefaultMaterial();
			idk.AddComponent<ECS::StaticMeshComponent>(EntOther, std::move(MeshCompx));

			idk.MarkRenderStateDirty(EntOther);
		}

		NewScene::Scene otherscene = Engine.CreateScene();
		NewScene::SceneEntity& EntOther = *otherscene.CreateEntity("hej");
		{
			ECS::TransformComponent idktf;
			idktf.SetTransform(DXM::Matrix::CreateRotationY(90.f) * DXM::Matrix::CreateScale(0.2, 0.3, 0.1) * DXM::Matrix::CreateTranslation(1, 0.5, 3));
			otherscene.AddComponent<ECS::TransformComponent>(EntOther, std::move(idktf));

			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = Engine.GetCubeMesh();
			MeshComp.m_MaterialReference = Engine.GetDefaultMaterial();
			otherscene.AddComponent<ECS::StaticMeshComponent>(EntOther, std::move(MeshComp));

			ECS::PointLightComponent PL;
			PL.SetPosition({ 5,0,0 });
			otherscene.AddComponent(EntOther, std::move(PL));

			ECS::CameraComponent Cam;
			Cam.m_TopLeft = { 0,0 };
			Cam.m_Dimensions = x->GetDimensions();
			Cam.m_NearPlane = 0.01f;
			Cam.m_FarPlane = 1000.f;
			Cam.m_Fov = 3.14 / 2.f;
			otherscene.AddComponent(EntOther, std::move(Cam));

			otherscene.MarkRenderStateDirty(EntOther);
		}

		x->Hide();

		while (ActiveWindow->IsOpen() && x->IsOpen())
		{
			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			ActiveWindow->HandleMessages();
			x->HandleMessages();

			Engine.BeginFrame();

			// Example
			static float Rot = 90.f;
			if (GetAsyncKeyState('1'))
			{
				Rot = 90.f;
			}

			if (GetAsyncKeyState('W'))
			{
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(0, 0, 0.001f);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState('S'))
			{
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(0, 0, -0.001f);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState('A'))
			{
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(0.001f, 0, 0);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState('D'))
			{
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(-0.001f, 0, 0);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState(VK_SPACE))
			{
				static bool IsHidden = true;
				if (IsHidden)
				{
					x->Show();
				}
				else
				{
					x->Hide();
				}

				IsHidden = !IsHidden;
			}

			Rot += 0.0002f;
			ECS::TransformComponent* TF = idk.GetComponent<ECS::TransformComponent>(EntIDK);
			TF->SetTransform(DXM::Matrix::CreateRotationY(-Rot) * DXM::Matrix::CreateScale(0.2) * DXM::Matrix::CreateTranslation(0, -0.5, 1));
			idk.MarkRenderStateDirty(EntIDK);

			Engine.Render(idk, ActiveWindow);
			Engine.Render(otherscene, x);

			Engine.EndFrame();
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