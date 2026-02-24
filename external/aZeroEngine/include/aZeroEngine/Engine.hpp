#pragma once
#include <memory>
#include "window/RenderWindow.hpp"
#include "renderer/Renderer.hpp"
#include "assets/AssetManager.hpp"
#include "scene\Scene.hpp"

namespace aZero
{
	class Engine : public NonCopyable
	{
	public:
		Engine(uint32_t bufferCount, const std::string& contentPath);
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;
		~Engine();

		IDxcCompilerX& GetCompiler() const { return *m_Compiler.p; }

		Asset::AssetManager& GetNewAssetManager() { return *m_NewAssetManager.get(); }
		Rendering::Renderer& GetRenderer() { return *m_Renderer.get(); }

		// todo Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		const std::string& GetProjectDirectory() const { return m_ProjectDirectory; }

	private:
		Microsoft::WRL::ComPtr<ID3D12DeviceX> m_Device;
		CComPtr<IDxcCompilerX> m_Compiler;

		// todo Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		std::string m_ProjectDirectory;

		// API Interfaces
		std::unique_ptr<Rendering::Renderer> m_Renderer;
		std::unique_ptr<Asset::AssetManager> m_NewAssetManager;
		//
	};
}