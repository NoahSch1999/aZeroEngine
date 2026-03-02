#include "aZeroEngine/Engine.hpp"

namespace aZero
{
	Engine::Engine(uint32_t bufferCount, const std::string& contentPath)
	{
		m_ProjectDirectory = contentPath;

		HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_Device.GetAddressOf()));
		if (FAILED(hr))
		{
			throw std::runtime_error("Engine() => Failed to create ID3D12DeviceX");
		}

		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
		if (FAILED(hr))
		{
			throw std::runtime_error("Engine() => Failed to create compiler");
		}

		m_Renderer = std::make_unique<Rendering::Renderer>(m_Device.Get(), bufferCount, *m_Compiler.p);
	}

	Engine::~Engine()
	{
		m_Renderer->FlushGPU();
	}
}