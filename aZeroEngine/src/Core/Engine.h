#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

// STD
#include <memory>
#include <string>

#include "Core/Renderer/D3D12Wrap/Window/Window.h"
#include "Core/D3D12Include.h"
#include "Scene/Scene.h"
#include "Renderer/RenderInterface.h"
#include "AssetTypes/Texture.h"

#define ASSET_PATH std::string("C:/Projects/Programming/aZeroEngine/aZeroEngine/assets/")

// https://shader-slang.com/slang/user-guide/get-started.html

namespace aZero
{
	class Engine
	{
	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		HINSTANCE m_AppInstance;

		ECS::EntityManager m_EntityManager;
		ECS::ComponentManagerDecl m_ComponentManager;

		std::unordered_map<std::string, Scene> m_Scenes;
		std::shared_ptr<Window> m_MainWindow = nullptr;
		
		std::unique_ptr<Rendering::Renderer> m_Renderer;

	public:
		Asset::Mesh Mesh;
		Asset::Material Material;
		
		Engine(HINSTANCE AppInstance, const DXM::Vector2& WindowResolution)
			:m_AppInstance(AppInstance)
		{
			HRESULT Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_Device.GetAddressOf()));
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

			m_Renderer = std::make_unique<Rendering::Renderer>(m_Device.Get(), WindowResolution);

			// Temp stuff
			m_MainWindow = std::make_shared<Window>(
				m_Renderer->GetGraphicsQueue(),
				m_AppInstance,
				WndProc,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				WindowResolution,
				false,
				"aZero Engine");

			m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(MAX_ENTITIES);
			m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MAX_ENTITIES);
			m_ComponentManager.GetComponentArray<TickComponent_Interface>().Init(MAX_ENTITIES);

			Mesh.LoadFromFile(ASSET_PATH + "meshes/soldier.fbx");
			m_Renderer->m_Entity_To_Mesh.emplace(UINT_MAX, Mesh);
			m_Renderer->MarkRenderStateDirty(Mesh);

			std::shared_ptr<Asset::Texture> Texture = std::make_shared<Asset::Texture>();
			Texture->LoadFromFile(ASSET_PATH + "textures/toySolderAlbedo.png");
			m_Renderer->MarkRenderStateDirty(*Texture.get());

			Texture->m_DescriptorIndex = 1; // Test to set

			Material.SetAlbedoTexture(Texture);
			Material.SetColor({ 1,2,3 });
			m_Renderer->MarkRenderStateDirty(Material);

			m_Renderer->m_GraphicsQueue.ExecuteContext(m_Renderer->m_GraphicsCommandContext);
		}

		void BeginFrame()
		{
			if (m_MainWindow->IsOpen())
			{
				m_MainWindow->HandleMessages();
			}

			if (m_MainWindow->IsOpen())
			{
				if (m_MainWindow->GetClientDimensions().x != m_Renderer->GetRenderResolution().x
					|| m_MainWindow->GetClientDimensions().y != m_Renderer->GetRenderResolution().y)
				{
					m_Renderer->FlushImmediate();
					m_Renderer->ChangeRenderResolution(m_MainWindow->GetClientDimensions());

					// NOTE: No resource input for the new bbs recycler here... but fine since we flush before anyways (?)
					m_MainWindow->GetSwapChain().Resize(m_MainWindow->GetClientDimensions());
				}
			}
		}

		void Update()
		{

		}

		void Render()
		{
			m_Renderer->BeginFrame();

			m_Renderer->Render(m_MainWindow->GetSwapChain());

			m_Renderer->EndFrame();
		}

		void FlushRenderingCommands()
		{
			m_Renderer->FlushImmediate();
		}

		std::shared_ptr<Window> GetWindow() { return m_MainWindow; }

		Scene* CreateScene(const std::string& Name)
		{
			if (m_Scenes.count(Name) == 0)
			{
				Rendering::RenderScene& RenderScene = m_Renderer->CreateScene(Name);
				m_Scenes.emplace(Name, Scene(Name, RenderScene, m_EntityManager, m_ComponentManager));
				return &m_Scenes.at(Name);
			}
			
			return nullptr;
		}

		void SetScene(Scene* InScene)
		{
			m_Renderer->SetScene(InScene);
		}

		Rendering::RenderInterface CreateRenderInterface()
		{
			return Rendering::RenderInterface(*m_Renderer.get(), m_ComponentManager);
		}

		ECS::ComponentManagerDecl& GetComponentManager() { return m_ComponentManager; }
	};
}