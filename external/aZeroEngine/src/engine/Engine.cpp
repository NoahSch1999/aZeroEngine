#include "aZeroEngine/Engine.hpp"

namespace aZero
{
	Engine::Engine(uint32_t bufferCount, const std::string& contentPath)
	{
		m_ProjectDirectory = contentPath;

		HRESULT Hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_Device.GetAddressOf()));
		if (FAILED(Hr))
		{
			throw std::runtime_error("Engine() => Failed to create ID3D12Device");
		}

		//m_Renderer = std::make_unique<Rendering::Renderer>(m_Device.Get(), bufferCount, *m_PipelineManager.get());
		m_AssetManager = std::make_unique<Asset::AssetManager>();
		m_SceneManager = std::make_unique<Scene::SceneManager>();
		m_PipelineManager = std::make_unique<Pipeline::PipelineManager>(m_Device.Get(), m_ProjectDirectory + SHADER_SOURCE_RELATIVE_PATH);
	}

	Engine::~Engine()
	{
		m_Renderer->FlushGraphicsQueue();
	}

	Rendering::RenderContext Engine::GetRenderContext()
	{
		return Rendering::RenderContext(*m_Renderer.get());
	}

	std::shared_ptr<Window::RenderWindow> Engine::CreateRenderWindow(DXM::Vector2 dimensions, const std::string& name)
	{
		const HWND ExistingWindow = FindWindowA(name.c_str(), name.c_str());
		if (ExistingWindow != NULL)
		{
			return nullptr;
		}

		return std::make_shared<Window::RenderWindow>(
			m_Renderer->m_GraphicsQueue,
			name,
			dimensions,
			&m_Renderer->m_ResourceRecycler,
			m_Renderer->m_BufferCount
		);
	}

	Rendering::RenderSurface Engine::CreateRenderSurface(
		const DXM::Vector2& dimensions,
		Rendering::RenderSurface::Type type,
		std::optional<DXM::Vector4> clearColor
	)
	{
		if (type == Rendering::RenderSurface::Type::Color_Target)
		{
			return Rendering::RenderSurface(
				m_Device.Get(),
				&m_Renderer->m_ResourceRecycler,
				m_Renderer->m_RTVHeap.GetDescriptor(),
				dimensions,
				type,
				clearColor
			);
		}

		return Rendering::RenderSurface(
			m_Device.Get(),
			&m_Renderer->m_ResourceRecycler,
			m_Renderer->m_DSVHeap.GetDescriptor(),
			dimensions,
			type,
			clearColor
		);
	}
}