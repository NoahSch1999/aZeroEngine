#pragma once
#include <memory>
#include "renderer/Renderer.hpp"
#include "scene/Scene.hpp"

namespace aZero
{
	class Engine : public NonCopyable
	{
	public:
		Engine(uint32_t bufferCount, const std::string& contentPath);
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;
		~Engine();

		IDxcCompilerX& GetCompiler() const { return *m_Compiler.Get(); }

		Rendering::Renderer& GetRenderer() const { return *m_Renderer.get(); }

		Rendering::RenderTarget CreateRenderTarget(const Rendering::RenderTargetDesc& desc, bool shouldClear) const
		{
			return Rendering::RenderTarget(m_Device.Get(), m_Renderer->m_RTVHeapNew, m_Renderer->m_NewResourceRecycler, desc, shouldClear);
		}

		Rendering::DepthTarget CreateDepthStencilTarget(const Rendering::DepthTargetDesc& desc, bool shouldClear) const
		{
			return Rendering::DepthTarget(m_Device.Get(), m_Renderer->m_DSVHeapNew, m_Renderer->m_NewResourceRecycler, desc, shouldClear);
		}

		// todo Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		const std::string& GetProjectDirectory() const { return m_ProjectDirectory; }

	private:
		Microsoft::WRL::ComPtr<ID3D12DeviceX> m_Device;
		Microsoft::WRL::ComPtr<IDxcCompilerX> m_Compiler;

		// todo Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		std::string m_ProjectDirectory;

		// API Interfaces
		std::unique_ptr<Rendering::Renderer> m_Renderer;
		//
	};
}