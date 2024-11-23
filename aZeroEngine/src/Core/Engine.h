#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

// STD
#include <memory>
#include <string>

#include "Core/Renderer/D3D12Wrap/Window/Window.h"
#include "Core/D3D12Core.h"
#include "Scene/Scene.h"
#include "Caches/MaterialCache.h"

#define ASSET_PATH std::string("C:/Projects/Programming/aZeroEngine/aZeroEngine/assets/")

namespace aZero
{
	class Engine
	{
		/*
		
		JOB:
		Interface between USER and systems (RENDERER, PHYSICS, etc...)
			Done through ex. the following:
				-Settings (RENDERER, PHYSICS, etc...)
				-ECS managing
				-Scene managing
				-Asset managing
		*/

	private:

		HINSTANCE m_AppInstance;
		ECS::EntityManager m_EntityManager;
		ECS::ComponentManagerDecl m_ComponentManager;

		Asset::MaterialCache<Asset::MaterialTemplate<Asset::BasicMaterialRenderData>> m_BasicMaterialCache;
		std::unordered_map<std::string, Scene> m_Scenes;
		std::shared_ptr<Window> m_MainWindow = nullptr;
		
	public:

		Engine(HINSTANCE AppInstance, const DXM::Vector2& WindowResolution)
			:m_AppInstance(AppInstance)
		{
			HRESULT Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(gDevice.GetAddressOf()));
			if (FAILED(Hr))
			{
				// DEBUG MACRO
				throw;
			}

			Hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
			if (FAILED(Hr))
			{
				throw;
			}

			gComponentManager.Init(m_ComponentManager);

			// Setup Renderer
			gRenderer.Init(*gComponentManager.m_ComponentManager, WindowResolution/* NOTE - This should be a setting so that the render res can be lower than window */);
			//

			// Setup default assets
			gRenderer.m_GraphicsCommandContext.StartRecording();
			gRenderer.m_PackedGPULookupBufferUpdateContext.StartRecording();

			gRenderer.m_PackedGPULookupBufferUpdateContext.StopRecording();
			gRenderer.m_GraphicsQueue.ExecuteContext({ gRenderer.m_PackedGPULookupBufferUpdateContext });

			gRenderer.m_Meshes.LoadFromFile(ASSET_PATH + "meshes/Torus.fbx", gRenderer.m_GraphicsCommandContext.GetCommandList());
			gRenderer.m_TextureFileAssets.LoadFromFile(ASSET_PATH + "textures/chunli.png", gRenderer.m_GraphicsCommandContext.GetCommandList());

			gRenderer.m_GraphicsCommandContext.StopRecording();
			gRenderer.m_GraphicsQueue.ExecuteContext({ gRenderer.m_GraphicsCommandContext });
			//

			// Setup ECS
			gComponentManager.GetComponentArray<ECS::TransformComponent>().Init(MAX_ENTITIES);
			gComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MAX_ENTITIES);
			gComponentManager.GetComponentArray<TickComponent_Interface>().Init(MAX_ENTITIES);
			//

			m_MainWindow = std::make_shared<Window>(
				gRenderer.GetGraphicsQueue(),
				m_AppInstance,
				WndProc,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				WindowResolution,
				false,
				"aZero Engine");
		}
		
		~Engine()
		{

		}

		void BeginFrame()
		{
			if (m_MainWindow->IsOpen())
			{
				m_MainWindow->HandleMessages();
			}

			if (m_MainWindow->IsOpen())
			{
				if (m_MainWindow->GetClientDimensions().x != gRenderer.GetRenderResolution().x
					|| m_MainWindow->GetClientDimensions().y != gRenderer.GetRenderResolution().y)
				{
					gRenderer.FlushImmediate();
					gRenderer.ChangeRenderResolution(m_MainWindow->GetClientDimensions());
					m_MainWindow->GetSwapChain().Resize(m_MainWindow->GetClientDimensions());
				}
			}
		}

		void Update()
		{

		}

		void Render()
		{
			gRenderer.BeginFrame();

			gRenderer.Render(m_MainWindow->GetSwapChain());

			gRenderer.EndFrame();
		}

		std::shared_ptr<Window> GetWindow() { return m_MainWindow; }

		Scene* CreateScene(const std::string& Name)
		{
			if (m_Scenes.count(Name) == 0)
			{
				m_Scenes.emplace(Name, Scene(Name, m_EntityManager));
				return &m_Scenes.at(Name);
			}
			
			return nullptr;
		}

		void SetScene(Scene* InScene)
		{
			gRenderer.SetScene(InScene);
		}
	};
}