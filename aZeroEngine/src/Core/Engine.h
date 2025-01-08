#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

// STD
#include <memory>
#include <string>

#include "Core/Renderer/D3D12Wrap/Window/Window.h"
#include "Scene/Scene.h"
#include "Renderer/RenderInterface.h"

namespace aZero
{
	class Engine
	{
	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		HINSTANCE m_AppInstance;
		
		std::unique_ptr<Rendering::Renderer> m_Renderer;

		std::shared_ptr<Asset::Mesh> m_DefaultCube;
		std::shared_ptr<Asset::Texture> m_DefaultTexture;
		std::shared_ptr<Asset::Material> m_DefaultMaterial;

		void LoadDefaultAssets()
		{
			m_DefaultCube = m_Renderer->m_AssetManager->CreateAsset<Asset::Mesh>();
			m_DefaultCube->LoadFromFile(ASSET_PATH + "meshes/defaultCube.fbx");
			m_Renderer->MarkRenderStateDirty(m_DefaultCube);

			m_DefaultTexture = m_Renderer->m_AssetManager->CreateAsset<Asset::Texture>();
			m_DefaultTexture->LoadFromFile(ASSET_PATH + "textures/defaultTexture.jpg");
			m_Renderer->MarkRenderStateDirty(m_DefaultTexture);

			m_DefaultMaterial = m_Renderer->m_AssetManager->CreateAsset<Asset::Material>();
			Asset::MaterialData MatData;
			MatData.m_AlbedoTexture = m_DefaultTexture;
			m_DefaultMaterial->SetData(std::move(MatData));
			m_Renderer->MarkRenderStateDirty(m_DefaultMaterial);
		}

	public:
		Engine(HINSTANCE AppInstance, const DXM::Vector2& WindowResolution, uint32_t BufferCount)
			:m_AppInstance(AppInstance)
		{
			HRESULT Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_Device.GetAddressOf()));
			if (FAILED(Hr))
			{
				throw std::runtime_error("Engine() => Failed to create ID3D12Device");
			}

			Hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
			if (FAILED(Hr))
			{
				throw std::runtime_error("Engine() => Failed to call CoInitializeEx()");
			}

			m_Renderer = std::make_unique<Rendering::Renderer>(m_Device.Get(), WindowResolution, BufferCount);

			this->LoadDefaultAssets();

			m_Renderer->m_GraphicsQueue.ExecuteContext(m_Renderer->m_GraphicsCommandContext);
		}

		~Engine()
		{
			this->FlushRenderingCommands();
		}

		void BeginFrame()
		{
			m_Renderer->BeginFrame();
		}

		void Render(NewScene::Scene& Scene, std::shared_ptr<Window::RenderWindow> Window)
		{
			m_Renderer->Render(Scene, Window);
		}

		void EndFrame()
		{
			m_Renderer->EndFrame();
		}

		void FlushRenderingCommands()
		{
			m_Renderer->FlushImmediate();
		}

		std::shared_ptr<Window::RenderWindow> CreateRenderWindow(DXM::Vector2 Dimensions, const std::string& Name)
		{
			// TODO: Add handling if name is already taken
			return std::make_shared<Window::RenderWindow>(
				m_AppInstance,
				m_Renderer->m_GraphicsQueue,
				Name,
				Dimensions,
				&m_Renderer->m_ResourceRecycler,
				m_Renderer->m_BufferCount
				);
		}

		NewScene::Scene CreateScene()
		{
			static NewScene::SceneID NextId = 0;
			const NewScene::SceneID NewId = NextId;
			NextId++;
			return NewScene::Scene(NewId, m_Device.Get());
		}

		Rendering::RenderInterface CreateRenderInterface()
		{
			return Rendering::RenderInterface(*m_Renderer.get());
		}

		std::shared_ptr<Asset::Mesh> GetCubeMesh() { return m_DefaultCube; }

		std::shared_ptr<Asset::Material> GetDefaultMaterial() { return m_DefaultMaterial; }
	};
}