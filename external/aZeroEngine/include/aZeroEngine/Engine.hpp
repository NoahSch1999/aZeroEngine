#pragma once
#include <memory>
#include "window/RenderWindow.hpp"
#include "renderer/RenderContext.hpp"
#include "pipeline/PipelineManager.hpp"

namespace aZero
{
	class Engine : public NonCopyable
	{
	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		CComPtr<IDxcCompiler3> m_Compiler;

		// todo Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		std::string m_ProjectDirectory;

		// API Interfaces
		std::unique_ptr<Rendering::Renderer> m_Renderer;
		std::unique_ptr<Asset::AssetManager> m_AssetManager; // todo Remove
		std::unique_ptr<Asset::NewAssetManager> m_NewAssetManager;
		std::unique_ptr<Scene::SceneManager> m_SceneManager;
		//

	public:
		Engine(uint32_t bufferCount, const std::string& contentPath);
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;
		~Engine();

		// todo Remove the windowing part of the API and let the user copy the backbuffer to their own swapchain
		std::shared_ptr<Window::RenderWindow> CreateRenderWindow(DXM::Vector2 dimensions, const std::string& name);

		// todo Replace with a better interface
		std::shared_ptr<Rendering::RenderSurface> CreateRenderSurface(
			const DXM::Vector2& dimensions,
			Rendering::RenderSurface::Type type,
			std::optional<DXM::Vector4> clearColor = std::optional<DXM::Vector4>{});

		Rendering::RenderContext GetRenderContext();

		IDxcCompiler3& GetCompiler() const { return *m_Compiler.p; }

		Asset::AssetManager& GetAssetManager() { return *m_AssetManager.get(); } // todo Remove

		Asset::NewAssetManager& GetNewAssetManager() { return *m_NewAssetManager.get(); }
		
		Scene::SceneManager& GetSceneManager() { return *m_SceneManager.get(); }

		// todo Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		const std::string& GetProjectDirectory() const { return m_ProjectDirectory; }

		ID3D12Device* GetDevice() const { return m_Device.Get(); }
	};
}