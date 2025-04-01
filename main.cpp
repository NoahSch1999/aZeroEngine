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
-Fix primbatch draw

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

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
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
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // IF using Docking Branch

		aZero::Engine Engine({ 1920, 1080 }, 3, aZero::Helper::GetProjectDirectory() + "/../../../content");
		Rendering::RenderContext RenderContext = Engine.GetRenderContext();

		std::shared_ptr<aZero::Window::RenderWindow> ActiveWindow = Engine.CreateRenderWindow({ 1920, 1080 }, "aZero Engine");

		ImGui_ImplWin32_Init(ActiveWindow->GetHandle());

		ImGui_ImplDX12_InitInfo Info;
		Info.CommandQueue = RenderContext.GetGraphicsQueue().GetCommandQueue();
		Info.Device = Engine.GetDevice();
		Info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		Info.NumFramesInFlight = RenderContext.GetFramesInFlight();
		Info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		Info.SrvDescriptorHeap = Engine.GetSRVDescriptorHeap().GetDescriptorHeap();

		static D3D12::DescriptorHeap* Heap = &Engine.GetSRVDescriptorHeap();
		static D3D12::Descriptor Desc;
		Info.SrvDescriptorAllocFn =
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
			{
				Desc = Heap->GetDescriptor();
				*out_cpu_handle = Desc.GetCPUHandle();
				*out_gpu_handle = Desc.GetGPUHandle();
			};
		Info.SrvDescriptorFreeFn =
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
			{
				Heap->RecycleDescriptor(cpu_handle, gpu_handle);
			};

		ImGui_ImplDX12_Init(&Info);

		//std::shared_ptr<aZero::Window::RenderWindow> x = Engine.CreateRenderWindow({ 800, 600 }, "TestScene1");
		Asset::RenderAssetManager& AssetMan = RenderContext.GetAssetManager();

		std::shared_ptr<Asset::Mesh> GoblinMesh = AssetMan.CreateAsset<Asset::Mesh>();
		GoblinMesh->LoadFromFile(aZero::Helper::GetProjectDirectory() + "/../../../content" + MESH_ASSET_RELATIVE_PATH + "goblin.fbx");
		RenderContext.MarkRenderStateDirty(GoblinMesh);

		std::shared_ptr<Asset::Texture> GoblinAlbedo = AssetMan.CreateAsset<Asset::Texture>();
		GoblinAlbedo->LoadFromFile(aZero::Helper::GetProjectDirectory() + "/../../../content" + TEXTURE_ASSET_RELATIVE_PATH + "goblinAlbedo.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		RenderContext.MarkRenderStateDirty(GoblinAlbedo);

		std::shared_ptr<Asset::Texture> GoblinNorm = AssetMan.CreateAsset<Asset::Texture>();
		GoblinNorm->LoadFromFile(aZero::Helper::GetProjectDirectory() + "/../../../content" + TEXTURE_ASSET_RELATIVE_PATH + "goblinNormals.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
		RenderContext.MarkRenderStateDirty(GoblinNorm);

		std::shared_ptr<Asset::Material> GoblinMat = AssetMan.CreateAsset<Asset::Material>();
		Asset::MaterialData MatData;
		MatData.m_AlbedoTexture = GoblinAlbedo;
		MatData.m_NormalMap = GoblinNorm;
		GoblinMat->SetData(std::move(MatData));
		RenderContext.MarkRenderStateDirty(GoblinMat);

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
			Cam.m_Dimensions = ActiveWindow->GetClientDimensions();
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
			Cam.m_Dimensions = /*x->GetDimensions()*/{ 1,1 };
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

		//x->Hide();

		Rendering::RenderSurface SceneColorSurface(
			Engine.CreateRenderSurface(ActiveWindow->GetBackBufferDimensions(), 
				Rendering::RenderSurface::Type::Color_Target, DXM::Vector4(0.2,0.2,0.2,0)));
		
		Rendering::RenderSurface SceneDepthSurface(
			Engine.CreateRenderSurface(ActiveWindow->GetBackBufferDimensions(), 
				Rendering::RenderSurface::Type::Depth_Target));

		bool open = true;
		bool IsFullScreen = true;
		ActiveWindow->SetFullscreenMode(IsFullScreen);
		while (ActiveWindow->IsOpen() /*&& x->IsOpen()*/)
		{
			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			//x->HandleMessages();

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			io.DisplaySize.x = 1337;
			io.DisplaySize.y = 420;

			ActiveWindow->HandleMessages();

			RenderContext.BeginRenderFrame();

			ImGui::ShowDemoWindow(&open);

			ImGui::Begin("IDK");
			if (ImGui::Button("HEJHEJ"))
			{
				printf("hej");
			}
			ImGui::End();

			ImGui::Begin("x");
			if (ImGui::Button("HEJHEJ"))
			{
				printf("hej");
			}
			ImGui::End();

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

			if (GetAsyncKeyState('D'))
			{
				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Position += DXM::Vector3(-0.01f, 0, 0);
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('A'))
			{
				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Position += DXM::Vector3(0.01f, 0, 0);
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('X'))
			{
				RenderContext.FlushRenderingCommands();
				ActiveWindow->Resize({ 500, 500 });

				ECS::CameraComponent* Cam = TestScene1.GetComponent<ECS::CameraComponent>(EntTestScene1);
				Cam->m_Dimensions = { 500, 500 };
				TestScene1.MarkRenderStateDirty(EntTestScene1);
			}

			if (GetAsyncKeyState('C'))
			{
				Engine.RebuildPipeline();
			}

			if (GetAsyncKeyState('F'))
			{
				ActiveWindow->SetFullscreenMode(IsFullScreen);
				IsFullScreen = !IsFullScreen;
			}

			ActiveWindow->WaitOnSwapchain();
			RenderContext.Render(TestScene1, SceneColorSurface, true, SceneDepthSurface, true);
			//Engine.Render(TestScene1.GetObjects<Scene::Scene::Camera>(). {&Batch}, ActiveWindow);
			//Engine.Render(otherscene, {}, x);

			auto ContextHandle = RenderContext.GetCommandContext();
			if (ContextHandle.has_value())
			{
				auto Context = ContextHandle.value().m_Context;
				ImGui::Render();
				ID3D12DescriptorHeap* HeapTemp = Heap->GetDescriptorHeap();
				Context->GetCommandList()->SetDescriptorHeaps(1, &HeapTemp);
				auto RTV = SceneColorSurface.GetView<D3D12::RenderTargetView>().GetDescriptorHandle();
				Context->GetCommandList()->OMSetRenderTargets(1, &RTV, false, nullptr);
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context->GetCommandList());
				RenderContext.GetGraphicsQueue().ExecuteContext(*Context);
			}

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				//ImGui::UpdatePlatformWindows();
				//ImGui::RenderPlatformWindowsDefault();
			}

			RenderContext.CompleteRender(SceneColorSurface, ActiveWindow);

			RenderContext.EndRenderFrame();

			ActiveWindow->Present();

		}
		Desc.~Descriptor();
		RenderContext.FlushRenderingCommands();
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	catch (std::invalid_argument& e)
	{
		printf(e.what());
		DebugBreak();
	}

	// Link error...
	//DEBUG_FUNC([&] {idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)); });

	int x = 2;
	x++;

	return 0;
}