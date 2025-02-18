#include "aZeroEditor.hpp"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }


using namespace aZero;
using namespace Rendering;

/*
std::numbers::pi;
map.contains(2);
[[likely]] case 2:
[[unlikely]] case 2:
(true && ... && args);
[[nodiscard]]


TODO-LIST:
Prio:
-Skeletal animation and components which works with octree and frustrum culling
-Basic PBR with normal map, metallic, roughness
	https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
	https://boksajak.github.io/files/CrashCourseBRDF.pdf
	https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
-Physics with rigidbody component
-Octree culling (with static/movable objects etc)
-Audio system with audio component
-Parenting system for entities

Non-prio:
-Quality render target up-scaling using nvidia or amd api (renders to lower resolution buffer and upscales)
-Render graph (in-progress)
-Level editor using the library api

TO-FIX:
-Fix light bounding spheres since we now define the "range" via a mathematical function
-Fix broken normal map
*/

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
		aZero::Engine Engine(instance, { 1920, 1080 }, 2, aZero::Helper::GetProjectDirectory() + "/../../../content");
		std::shared_ptr<aZero::Window::RenderWindow> ActiveWindow = Engine.CreateRenderWindow({ 1920, 1080 }, "aZero Engine");
		std::shared_ptr<aZero::Window::RenderWindow> x = Engine.CreateRenderWindow({ 800, 600 }, "TestScene1");
		Rendering::RenderInterface RenderInterface = Engine.CreateRenderInterface();
		Asset::RenderAssetManager& AssetMan = RenderInterface.GetAssetManager();

		std::shared_ptr<Asset::Mesh> GoblinMesh = AssetMan.CreateAsset<Asset::Mesh>();
		GoblinMesh->LoadFromFile(aZero::Helper::GetProjectDirectory() + "/../../../content" + MESH_ASSET_RELATIVE_PATH + "goblin.fbx");
		RenderInterface.MarkRenderStateDirty(GoblinMesh);

		std::shared_ptr<Asset::Texture> GoblinAlbedo = AssetMan.CreateAsset<Asset::Texture>();
		GoblinAlbedo->LoadFromFile(aZero::Helper::GetProjectDirectory() + "/../../../content" + TEXTURE_ASSET_RELATIVE_PATH + "goblinAlbedo.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		RenderInterface.MarkRenderStateDirty(GoblinAlbedo);

		std::shared_ptr<Asset::Texture> GoblinNorm = AssetMan.CreateAsset<Asset::Texture>();
		GoblinNorm->LoadFromFile(aZero::Helper::GetProjectDirectory() + "/../../../content" + TEXTURE_ASSET_RELATIVE_PATH + "goblinNormals.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		RenderInterface.MarkRenderStateDirty(GoblinNorm);

		std::shared_ptr<Asset::Material> GoblinMat = AssetMan.CreateAsset<Asset::Material>();
		Asset::MaterialData MatData;
		MatData.m_AlbedoTexture = GoblinAlbedo;
		MatData.m_NormalMap = GoblinNorm;
		GoblinMat->SetData(std::move(MatData));
		RenderInterface.MarkRenderStateDirty(GoblinMat);

		Scene::Scene TestScene1 = Engine.CreateScene();
		Scene::SceneEntity& EntTestScene1 = *TestScene1.CreateEntity("hej");
		Scene::SceneEntity& Ent2TestScene1 = *TestScene1.CreateEntity("hej");
		{
			ECS::TransformComponent TestScene1tfs;
			TestScene1tfs.SetTransform(DXM::Matrix::CreateTranslation(0, 0, 3));

			TestScene1.AddComponent<ECS::TransformComponent>(EntTestScene1, std::move(TestScene1tfs));

			ECS::StaticMeshComponent MeshCompx;
			MeshCompx.m_MeshReference = Engine.GetCubeMesh();
			MeshCompx.m_MaterialReference = Engine.GetDefaultMaterial();
			TestScene1.AddComponent<ECS::StaticMeshComponent>(EntTestScene1, std::move(MeshCompx));

			ECS::CameraComponent Cam;
			Cam.m_TopLeft = { 0,0 };
			Cam.m_Dimensions = ActiveWindow->GetDimensions();
			Cam.m_NearPlane = 0.001f;
			Cam.m_FarPlane = 1000.f;
			Cam.m_Fov = 3.14 / 2.f;
			TestScene1.AddComponent(EntTestScene1, std::move(Cam));

			ECS::DirectionalLightComponent DLight;
			DirectionalLightData DLightData;
			DLightData.Color = { 1,1,1 };
			DLightData.Direction = { 0,0,1 };
			DLightData.Intensity = 0.4;
			DLight.SetData(DLightData);
			TestScene1.AddComponent<ECS::DirectionalLightComponent>(EntTestScene1, std::move(DLight));

			ECS::SpotLightComponent SLight;
			SpotLightData SLightData;
			SLightData.Color = { 1,0,0 };
			SLightData.Position = { 0, 0,-1 };
			SLightData.Direction = { 0,0,1 };
			SLightData.Intensity = 2;
			SLightData.CutoffAngle = 0.99;
			SLight.SetData(SLightData);
			TestScene1.AddComponent<ECS::SpotLightComponent>(EntTestScene1, std::move(SLight));

			ECS::PointLightComponent PLight;
			PointLightData PLightData;
			PLightData.Color = { 1,1,1 };
			PLightData.FalloffFactor = 1;
			PLightData.Intensity = 2;
			PLightData.Position = { 2,0,5 };
			PLight.SetData(PLightData);
			TestScene1.AddComponent<ECS::PointLightComponent>(EntTestScene1, std::move(PLight));

			TestScene1.MarkRenderStateDirty(EntTestScene1);
		}

		// Init 2nd scene
		Scene::Scene otherscene = Engine.CreateScene();
		Scene::SceneEntity& EntOther = *otherscene.CreateEntity("hej");
		{
			ECS::TransformComponent TestScene1tf;
			TestScene1tf.SetTransform(DXM::Matrix::CreateRotationY(3.14) * DXM::Matrix::CreateTranslation(0, -2, 5));
			otherscene.AddComponent<ECS::TransformComponent>(EntOther, std::move(TestScene1tf));

			ECS::StaticMeshComponent MeshComp;
			MeshComp.m_MeshReference = GoblinMesh;
			MeshComp.m_MaterialReference = GoblinMat;
			otherscene.AddComponent<ECS::StaticMeshComponent>(EntOther, std::move(MeshComp));

			ECS::PointLightComponent PL;
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

		Rendering::PrimitiveBatch Batch(Rendering::PrimitiveBatch::PrimitiveType::LINESTRIP, Rendering::PrimitiveBatch::RenderLayer::DEPTH);
		Batch.AddPoints(Rendering::PrimitiveBatch::Point({ DXM::Vector3(0,0,0),DXM::Vector3(1,0,0) }));
		Batch.AddPoints(Rendering::PrimitiveBatch::Point({ DXM::Vector3(1,0,0),DXM::Vector3(1,0,0) }));
		Batch.AddPoints(Rendering::PrimitiveBatch::Point({ DXM::Vector3(1,0,0),DXM::Vector3(0,1,0) }));
		Batch.AddPoints(Rendering::PrimitiveBatch::Point({ DXM::Vector3(0,1,0),DXM::Vector3(0,1,0) }));
		Batch.AddPoints(Rendering::PrimitiveBatch::Point({ DXM::Vector3(0,1,0),DXM::Vector3(0,0,1) }));
		Batch.AddPoints(Rendering::PrimitiveBatch::Point({ DXM::Vector3(0,0,0),DXM::Vector3(0,0,1) }));

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

			if (GetAsyncKeyState('W'))
			{
				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Position += DXM::Vector3(0, 0, 0.01f);
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('S'))
			{
				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Position += DXM::Vector3(0, 0, -0.03f);
				TestScene1.MarkRenderStateDirty(EntTestScene1);
				otherscene.MarkRenderStateDirty(EntOther);
			}

			if (GetAsyncKeyState('A'))
			{
				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Position += DXM::Vector3(-0.01f, 0, 0);
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('D'))
			{
				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Position += DXM::Vector3(0.01f, 0, 0);
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('X'))
			{
				Engine.FlushRenderingCommands();
				ActiveWindow->Resize({ 500, 500 });

				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Dimensions = { 500, 500 };
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('C'))
			{
				Engine.RebuildPipeline();
			}

			Engine.Render(TestScene1, {&Batch}, ActiveWindow);
			Engine.Render(otherscene, {}, x);

			Engine.EndFrame();
		}
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