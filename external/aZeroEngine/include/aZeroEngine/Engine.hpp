#pragma once
#include <memory>
#include "window/RenderWindow.hpp"
#include "renderer/RenderContext.hpp"
#include "pipeline/PipelineManager.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class PipelineManager;
	}

	class Engine : public NonCopyable, public NonMovable
	{
	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;

		// TODO: Replace
		std::string m_ProjectDirectory;

		// API Interfaces
		std::unique_ptr<Rendering::Renderer> m_Renderer;
		std::unique_ptr<Asset::AssetManager> m_AssetManager;
		std::unique_ptr<Scene::SceneManager> m_SceneManager;
		std::unique_ptr<Pipeline::PipelineManager> m_PipelineManager;
		//

	public:
		Engine(uint32_t bufferCount, const std::string& contentPath);

		~Engine();

		// TODO: Remove the windowing part of the API and let the user copy the backbuffer to their own swapchain
		std::shared_ptr<Window::RenderWindow> CreateRenderWindow(DXM::Vector2 dimensions, const std::string& name);

		Rendering::RenderSurface CreateRenderSurface(
			const DXM::Vector2& dimensions,
			Rendering::RenderSurface::Type type,
			std::optional<DXM::Vector4> clearColor = std::optional<DXM::Vector4>{});

		Rendering::RenderContext GetRenderContext();

		Asset::AssetManager& GetAssetManager() { return *m_AssetManager.get(); }
		
		Scene::SceneManager& GetSceneManager() { return *m_SceneManager.get(); }

		Pipeline::PipelineManager& GetPipelineManager() { return *m_PipelineManager.get(); }

		// TODO: Replace
		const std::string& GetProjectDirectory() const { return m_ProjectDirectory; }

		ID3D12Device* GetDevice() const { return m_Device.Get(); }
	};
}