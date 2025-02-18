#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

// STD
#include <memory>
#include <string>

#include "window/RenderWindow.hpp"
#include "scene/Scene.hpp"
#include "renderer/RenderInterface.hpp"

namespace aZero
{
	class Engine
	{
	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		HINSTANCE m_AppInstance;
		std::string m_ProjectDirectory;

		std::unique_ptr<Rendering::Renderer> m_Renderer;

		std::shared_ptr<Asset::Mesh> m_DefaultCube;
		std::shared_ptr<Asset::Texture> m_DefaultTexture;
		std::shared_ptr<Asset::Material> m_DefaultMaterial;

		void LoadDefaultAssets()
		{
			m_DefaultCube = m_Renderer->m_AssetManager->CreateAsset<Asset::Mesh>();
			m_DefaultCube->LoadFromFile(m_ProjectDirectory + MESH_ASSET_RELATIVE_PATH + "unitCube.fbx");
			m_Renderer->MarkRenderStateDirty(m_DefaultCube);

			m_DefaultTexture = m_Renderer->m_AssetManager->CreateAsset<Asset::Texture>();
			m_DefaultTexture->LoadFromFile(m_ProjectDirectory + TEXTURE_ASSET_RELATIVE_PATH + "chunli.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			m_Renderer->MarkRenderStateDirty(m_DefaultTexture);

			std::shared_ptr<Asset::Texture> NormalMap = m_Renderer->m_AssetManager->CreateAsset<Asset::Texture>();
			NormalMap->LoadFromFile(m_ProjectDirectory + TEXTURE_ASSET_RELATIVE_PATH + "testNormalMap.png", DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
			m_Renderer->MarkRenderStateDirty(NormalMap);

			m_DefaultMaterial = m_Renderer->m_AssetManager->CreateAsset<Asset::Material>();
			Asset::MaterialData MatData;
			MatData.m_AlbedoTexture = m_DefaultTexture;
			MatData.m_NormalMap = NormalMap;
			m_DefaultMaterial->SetData(std::move(MatData));
			m_Renderer->MarkRenderStateDirty(m_DefaultMaterial);
		}

	public:

		void RebuildPipeline()
		{
			this->FlushRenderingCommands();
			D3D12::RenderPass Pass;
			{
				D3D12::Shader BasePassVS;
				BasePassVS.CompileFromFile(this->m_Renderer->GetCompiler(), m_ProjectDirectory + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");

				D3D12::Shader BasePassPS;
				BasePassPS.CompileFromFile(this->m_Renderer->GetCompiler(), m_ProjectDirectory + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

				Pass.Init(m_Device.Get(), BasePassVS, BasePassPS, {this->m_Renderer->m_FinalRenderSurface.GetResource()->GetDesc().Format}, DXGI_FORMAT_D24_UNORM_S8_UINT);
			}

			m_Renderer->m_StaticMeshPass.m_Pass = std::move(Pass);
		}

		Engine(HINSTANCE AppInstance, const DXM::Vector2& WindowResolution, uint32_t BufferCount, const std::string& ContentPath)
			:m_AppInstance(AppInstance)
		{
			m_ProjectDirectory = ContentPath; // aZero::Helper::GetProjectDirectory();

			HRESULT Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_Device.GetAddressOf()));
			if (FAILED(Hr))
			{
				throw std::runtime_error("Engine() => Failed to create ID3D12Device");
			}

			m_Renderer = std::make_unique<Rendering::Renderer>(m_Device.Get(), WindowResolution, BufferCount, m_ProjectDirectory);

			this->LoadDefaultAssets();
		}

		~Engine()
		{
			this->FlushRenderingCommands();
		}

		void BeginFrame()
		{
			m_Renderer->BeginFrame();
		}

		void Render(Scene::Scene& Scene, const std::vector<Rendering::PrimitiveBatch*>& Batches, std::shared_ptr<Window::RenderWindow> Window)
		{
			m_Renderer->Render(Scene, Batches, Window);
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
			const HWND ExistingWindow = FindWindowA(Name.c_str(), Name.c_str());
			if (ExistingWindow != NULL)
			{
				return nullptr;
			}

			return std::make_shared<Window::RenderWindow>(
				m_AppInstance,
				m_Renderer->m_GraphicsQueue,
				Name,
				Dimensions,
				m_Renderer->m_RenderResolution,
				&m_Renderer->m_ResourceRecycler,
				m_Renderer->m_BufferCount
			);
		}

		Scene::Scene CreateScene()
		{
			static Scene::SceneID NextId = 0;
			const Scene::SceneID NewId = NextId;
			NextId++;
			return Scene::Scene(NewId, m_Device.Get());
		}

		Rendering::RenderInterface CreateRenderInterface()
		{
			return Rendering::RenderInterface(*m_Renderer.get());
		}

		const std::string& GetProjectDirectory() const
		{
			return m_ProjectDirectory;
		}

		std::shared_ptr<Asset::Mesh> GetCubeMesh() { return m_DefaultCube; }

		std::shared_ptr<Asset::Material> GetDefaultMaterial() { return m_DefaultMaterial; }
	};
}