#include "Core/Engine.h"

#include "Core/Renderer/RenderAssetManager.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;
using namespace Rendering;

/*
TODO-LIST:

FUNCTIONALITY:
!. Add easy imgui support when using the library in an app (includes easy texture asset viewing)

!. Implement resizing on-demands for all relevant gpu buffers (ex. scene buffers, frameallocator etc)
	Question: How to determine shrinking to save vram...?

!. Fix bounding boxes and culling for the lights

!. Test add and remove functionality for entities and gpu resources to check no mem leaks and if it works as expected

!. Rework render passes and how they're used which includes adding a simple post-processing system that can be used with lib

!. Fix broken normal map

!. Add animation component etc
	Includes skeleton asset, animation asset, weights, specific animation pass, skeletalmeshcomponent(?)
	Keep in mind to design so both frustrum and octree culling can be used with it. (maybe generate some max bounds based on animations added?)

!. Add input system

!. Add octree culling to scene and movable/static flag to scene entities which allow seperate paths for rendering and culling
	Takes rigidbodies into account

!. Add physics system and rigidbody component

!. Add audio system

!. Add parenting system

PROJECT:
!. Change folder structure so it follows a more professional structure (ex. src, include, etc...)

!. Make engine a static library

!. Make level editor exe app

!. Rework engine path system 
	Ex:
		paths to project assets
		paths to custom shaders (cached compiled and source) used in post proc pass system
	Path to engine core shaders (cached compiled and source)
	All paths should be read from a file thats stored relative to the project exe.
		This includes the core shader source (and cached) + the core assets

!. Change so that the library and editor can be cloned and generated using cmake or something similar so other IDEs can be used
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

		Scene::Scene idk = Engine.CreateScene();
		Scene::SceneEntity& EntIDK = *idk.CreateEntity("hej");
		Scene::SceneEntity& EntIDK3 = *idk.CreateEntity("hej");

		std::shared_ptr<Asset::Mesh> GoblinMesh = AssetMan.CreateAsset<Asset::Mesh>();
		GoblinMesh->LoadFromFile(ASSET_PATH + "meshes/goblin.fbx");
		RenderInterface.MarkRenderStateDirty(GoblinMesh);

		std::shared_ptr<Asset::Texture> GoblinAlbedo = AssetMan.CreateAsset<Asset::Texture>();
		GoblinAlbedo->LoadFromFile(ASSET_PATH + "textures/goblinAlbedo.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		RenderInterface.MarkRenderStateDirty(GoblinAlbedo);

		std::shared_ptr<Asset::Texture> GoblinNorm = AssetMan.CreateAsset<Asset::Texture>();
		GoblinNorm->LoadFromFile(ASSET_PATH + "textures/goblinNormals.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
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
			//idk.AddComponent<ECS::StaticMeshComponent>(EntIDK, std::move(MeshComp));

			ECS::CameraComponent Cam;
			Cam.m_TopLeft = { 0,0 };
			//Cam.m_Dimensions = { ActiveWindow->GetDimensions().x / 2.f, ActiveWindow->GetDimensions().y };
			Cam.m_Dimensions = ActiveWindow->GetDimensions();
			Cam.m_NearPlane = 0.001f;
			Cam.m_FarPlane = 1000.f;
			Cam.m_Fov = 3.14 / 2.f;
			idk.AddComponent(EntIDK, std::move(Cam));

			ECS::DirectionalLightComponent DLight;
			DirectionalLightData DLightData;
			DLightData.Color = { 1,1,1 };
			DLightData.Direction = { 0,0,1 };
			DLightData.Intensity = 0.4;
			DLight.SetData(DLightData);
			idk.AddComponent<ECS::DirectionalLightComponent>(EntIDK, std::move(DLight));

			/*ECS::PointLightComponent PLight;
			PointLightData PLightData;
			PLightData.Color = { 1,1,1 };
			PLightData.Position = { 2.1,0,4 };
			PLightData.Intensity = 2;
			PLightData.FalloffFactor = 2;
			PLight.SetData(PLightData);
			idk.AddComponent<ECS::PointLightComponent>(EntIDK, std::move(PLight));*/

			ECS::SpotLightComponent SLight;
			SpotLightData SLightData;
			SLightData.Color = { 1,1,1 };
			SLightData.Position = { 2, 0,-2 };
			SLightData.Direction = { 0,0,1 };
			SLightData.Intensity = 2;
			SLightData.CutoffAngle = 0.97;
			SLight.SetData(SLightData);
			idk.AddComponent<ECS::SpotLightComponent>(EntIDK, std::move(SLight));

			idk.MarkRenderStateDirty(EntIDK);
		}

		Scene::SceneEntity& EntOtherIDK = *idk.CreateEntity("hej");
		{
			ECS::TransformComponent idktf;
			idktf.SetTransform(DXM::Matrix::CreateRotationY(90.f) * DXM::Matrix::CreateScale(0.2) * DXM::Matrix::CreateTranslation(0, 0, 4));
			idk.AddComponent<ECS::TransformComponent>(EntIDK3, std::move(idktf));

			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = GoblinMesh;
			MeshComp.m_MaterialReference = GoblinMat;
			//idk.AddComponent<ECS::StaticMeshComponent>(EntIDK3, std::move(MeshComp));

			ECS::CameraComponent Cam;
			Cam.m_TopLeft = { ActiveWindow->GetDimensions().x / 2.f,0 };
			Cam.m_Dimensions = { ActiveWindow->GetDimensions().x / 2.f, ActiveWindow->GetDimensions().y };
			Cam.m_NearPlane = 0.001f;
			Cam.m_FarPlane = 1000.f;
			Cam.m_Fov = 3.14 / 2.f;
			//idk.AddComponent(EntIDK3, std::move(Cam));

			ECS::DirectionalLightComponent DLight;
			DirectionalLightData DLightData;
			DLightData.Color = { 1,1,1 };
			DLightData.Direction = { 0,0,1 };
			DLightData.Intensity = 0.4;
			DLight.SetData(DLightData);
			idk.AddComponent<ECS::DirectionalLightComponent>(EntIDK3, std::move(DLight));

			ECS::PointLightComponent PLight;
			PointLightData PLightData;
			PLightData.Color = { 1,1,1 };
			PLightData.Position = { 2.1,0,4 };
			PLightData.Intensity = 2;
			PLightData.FalloffFactor = 2;
			PLight.SetData(PLightData);
			idk.AddComponent<ECS::PointLightComponent>(EntIDK3, std::move(PLight));

			ECS::SpotLightComponent SLight;
			SpotLightData SLightData;
			SLightData.Color = { 1,1,1 };
			SLightData.Position = { 0,0,-2 };
			SLightData.Direction = { 0,0,1 };
			SLightData.Intensity = 2;
			SLightData.CutoffAngle = 0.9;
			SLightData.Range = 4;
			SLight.SetData(SLightData);
			idk.AddComponent<ECS::SpotLightComponent>(EntIDK3, std::move(SLight));

			idk.MarkRenderStateDirty(EntIDK3);

			ECS::TransformComponent idktfs;
			idktfs.SetTransform(
				/*DXM::Matrix::CreateRotationY(3.1415 * 1)* *//*DXM::Matrix::CreateScale(1, 1, 1)* */DXM::Matrix::CreateTranslation(0, 0, 5));

			idk.AddComponent<ECS::TransformComponent>(EntOtherIDK, std::move(idktfs));

			ECS::StaticMeshComponent MeshCompx;
			MeshCompx.m_MeshReference = GoblinMesh;
			MeshCompx.m_MaterialReference = GoblinMat;
			MeshCompx.m_MeshReference = Engine.GetCubeMesh();
			//MeshCompx.m_MaterialReference = Engine.GetDefaultMaterial();
			idk.AddComponent<ECS::StaticMeshComponent>(EntOtherIDK, std::move(MeshCompx));

			idk.MarkRenderStateDirty(EntOtherIDK);
		}

		Scene::Scene otherscene = Engine.CreateScene();
		Scene::SceneEntity& EntOther = *otherscene.CreateEntity("hej");
		{
			ECS::TransformComponent idktf;
			idktf.SetTransform(/*DXM::Matrix::CreateRotationY(90.f)*/ /** DXM::Matrix::CreateScale(0.2, 0.3, 0.1) **/ DXM::Matrix::CreateTranslation(1, 0.5, 3));
			otherscene.AddComponent<ECS::TransformComponent>(EntOther, std::move(idktf));

			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = Engine.GetCubeMesh();
			MeshComp.m_MaterialReference = Engine.GetDefaultMaterial();
			otherscene.AddComponent<ECS::StaticMeshComponent>(EntOther, std::move(MeshComp));

			ECS::PointLightComponent PL;
			//PL.SetPosition({ 5,0,0 });
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



		PrimitiveBatch Batch;
		{
			Batch.SetLayer(PrimitiveBatch::RenderLayer::DEPTH);
			Batch.SetPrimitiveType(PrimitiveBatch::PrimitiveType::LINE);
			PrimitiveBatch::Point P1;
			P1.Position = { 1, 1, 0 };
			P1.Color = { 1,0,0 };

			PrimitiveBatch::Point P2;
			P2.Position = { -1, 1, 0 };
			P2.Color = { 0,1,0 };

			PrimitiveBatch::Point P3;
			P3.Position = { -1, 1, 0 };
			P3.Color = { 0,0,1 };

			PrimitiveBatch::Point P4;
			P4.Position = { -1, -1, 0 };
			P4.Color = { 1,1,1 };

			PrimitiveBatch::Point P5;
			P5.Position = { -1, -1, 0 };
			P5.Color = { 1,0,0 };

			PrimitiveBatch::Point P6;
			P6.Position = { 1, -1, 0 };
			P6.Color = { 0,1,0 };

			PrimitiveBatch::Point P7;
			P7.Position = { 1, -1, 0 };
			P7.Color = { 0,0,1 };

			PrimitiveBatch::Point P8;
			P8.Position = { 1, 1, 0 };
			P8.Color = { 1,1,1 };

			Batch.AddPoints({ P1, P2, P3, P4, P5, P6, P7, P8 });
		}

		PrimitiveBatch Batchs;
		Batchs.SetLayer(PrimitiveBatch::RenderLayer::NO_DEPTH);
		Batchs.SetPrimitiveType(PrimitiveBatch::PrimitiveType::LINE);
		{
			PrimitiveBatch::Point P1;
			P1.Position = { 1, 1, 1 };
			P1.Color = { 1,0,0 };

			PrimitiveBatch::Point P2;
			P2.Position = { -1, 1, 1 };
			P2.Color = { 0,1,0 };

			PrimitiveBatch::Point P3;
			P3.Position = { -1, 1, 1 };
			P3.Color = { 0,0,1 };

			PrimitiveBatch::Point P4;
			P4.Position = { -1, -1, 1 };
			P4.Color = { 1,1,1 };

			PrimitiveBatch::Point P5;
			P5.Position = { -1, -1, 1 };
			P5.Color = { 1,0,0 };

			PrimitiveBatch::Point P6;
			P6.Position = { 1, -1, 1 };
			P6.Color = { 0,1,0 };

			PrimitiveBatch::Point P7;
			P7.Position = { 1, -1, 1 };
			P7.Color = { 0,0,1 };

			PrimitiveBatch::Point P8;
			P8.Position = { 1, 1, 1 };
			P8.Color = { 1,1,1 };

			Batchs.AddPoints({ P1, P2, P3, P4, P5, P6, P7, P8 });
		}

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


			//ECS::SpotLightComponent* SL = idk.GetComponent<ECS::SpotLightComponent>(EntIDK3);
			ECS::PointLightComponent* SL = idk.GetComponent<ECS::PointLightComponent>(EntIDK3);
			auto Data = SL->GetData();

			if (GetAsyncKeyState('W'))
			{
				//Data.Color = { 1,0,0 };
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(0, 0, 0.01f);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState('X'))
			{
				//Data.Color = { 1,1,1 };
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(0, 0, -0.03f);
				idk.MarkRenderStateDirty(EntIDK);
				otherscene.MarkRenderStateDirty(EntOther);
			}

			if (GetAsyncKeyState('A'))
			{
				//Data.Color = { 0,1,0 };
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(0.01f, 0, 0);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState('D'))
			{
				//Data.Color = { 0,0,1 };
				ECS::CameraComponent* Cam = idk.GetComponent<ECS::CameraComponent>(EntIDK);
				Cam->m_Position += DXM::Vector3(-0.01f, 0, 0);
				idk.MarkRenderStateDirty(EntIDK);
			}

			if (GetAsyncKeyState(VK_SPACE))
			{
				Engine.FlushRenderingCommands();
				Engine.CreatePipeline();

				/*static bool IsHidden = true;
				if (IsHidden)
				{
					x->Show();
				}
				else
				{
					x->Hide();
				}

				IsHidden = !IsHidden;*/
			}

			Rot += 0.00000001f;

			//Data.Position = idk.GetComponent<ECS::CameraComponent>(EntIDK)->m_Position;
			/*DXM::Matrix RotMat = DXM::Matrix::CreateRotationY(Rot);
			Data.Direction = DXM::Vector3::TransformNormal(Data.Direction, RotMat);
			Data.Direction.Normalize();*/

			SL->SetData(Data);
			idk.MarkRenderStateDirty(EntIDK3);

			Engine.Render(idk, {&Batch, &Batchs}, ActiveWindow);
			//Engine.Render(otherscene, x);

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