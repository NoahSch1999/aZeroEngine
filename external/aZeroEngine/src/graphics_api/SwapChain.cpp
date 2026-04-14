#include "SwapChain.hpp"

aZero::RenderAPI::SwapChain::SwapChain(HWND wndHandle, RenderAPI::CommandQueue& cmdQueue, const DXM::Vector2& backBufferDimensions, uint32_t numBackBuffers, DXGI_FORMAT format)
{
	// If it already was enable we sync to enable reinit without having to worry about the backbuffers being in use.
	// We also don't create the factory since it can be reused.
	if (m_SwapChain.Get())
	{
		cmdQueue.SignalAndFlush();
	}
	else
	{
#ifdef USE_DEBUG		// Enable debug layer on factory if compiled with USE_DEBUG
			const HRESULT dxqiRes = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_Factory.GetAddressOf()));
#else
			const HRESULT dxqiRes = CreateDXGIFactory2(0, IID_PPV_ARGS(m_Factory.GetAddressOf()));
#endif
			if (FAILED(dxqiRes))
			{
				throw std::invalid_argument("Failed to create DXGIFactory");
			}
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = static_cast<UINT>(backBufferDimensions.x);
	swapChainDesc.Height = static_cast<UINT>(backBufferDimensions.y);
	swapChainDesc.Format = format;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferCount = numBackBuffers;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
	DEVMODEA displayInfo;
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &displayInfo);

	fullscreenDesc.RefreshRate.Numerator = displayInfo.dmDisplayFrequency;
	fullscreenDesc.RefreshRate.Denominator = 1;
	fullscreenDesc.Windowed = true;
	fullscreenDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	const HRESULT swapChainRes = m_Factory->CreateSwapChainForHwnd(cmdQueue.Get(), wndHandle,
		&swapChainDesc, &fullscreenDesc, nullptr, (IDXGISwapChain1**)m_SwapChain.GetAddressOf());
	if (FAILED(swapChainRes))
	{
		throw std::invalid_argument("Failed to create swapchain");
	}

	this->ReleaseBuffers();

	m_BackBuffers.resize(numBackBuffers);

	this->PopulateBackBuffers();

	m_diCmdQueue = &cmdQueue;
}

aZero::RenderAPI::SwapChain::~SwapChain()
{
	// Note - This might be dangerous... might change this later...
	if (m_diCmdQueue)
	{
		m_diCmdQueue->SignalAndFlush();
	}
}

aZero::RenderAPI::SwapChain::SwapChain(SwapChain&& other) noexcept
{
	*this = std::move(other);
}

aZero::RenderAPI::SwapChain& aZero::RenderAPI::SwapChain::operator=(SwapChain&& other) noexcept
{
	std::swap(m_SwapChain, other.m_SwapChain);
	std::swap(m_Factory, other.m_Factory);
	std::swap(m_diCmdQueue, other.m_diCmdQueue);
	std::swap(m_BackBuffers, other.m_BackBuffers);
	std::swap(m_NextBackBuffer, other.m_NextBackBuffer);
	return *this;
}

void aZero::RenderAPI::SwapChain::Resize(RenderAPI::CommandQueue& cmdQueue, const DXM::Vector2& backBufferDimensions)
{
	cmdQueue.SignalAndFlush();

	this->ReleaseBuffers();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	m_SwapChain->GetDesc(&swapChainDesc);
	m_SwapChain->ResizeBuffers(
		static_cast<UINT>(m_BackBuffers.size()),
		static_cast<UINT>(backBufferDimensions.x), static_cast<UINT>(backBufferDimensions.y),
		swapChainDesc.BufferDesc.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);

	this->PopulateBackBuffers();
}

void aZero::RenderAPI::SwapChain::PopulateBackBuffers()
{
	for (uint32_t i = 0; i < m_BackBuffers.size(); i++)
	{
		m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_BackBuffers[i].GetAddressOf()));
#ifdef USE_DEBUG
		auto name = L"BB_Index: " + std::wstring(std::to_wstring(i));
		m_BackBuffers[i]->SetName(name.c_str());
#endif
	}

	m_NextBackBuffer = 0;
}