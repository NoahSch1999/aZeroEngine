#include "SwapChain.h"

aZero::D3D12::SwapChain::SwapChain(HWND WindowHandle, const CommandQueue& GraphicsQueue, DXGI_FORMAT BackBufferFormat, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
{
	ID3D12Device* Device;
	const HRESULT GetDeviceRes = GraphicsQueue.GetCommandQueue()->GetDevice(IID_PPV_ARGS(&Device));
	if (FAILED(GetDeviceRes))
	{
		throw std::invalid_argument("SwapChain::Constructor() => Failed to get input CommandQueue device");
	}

	const HRESULT DXGIRes = CreateDXGIFactory(IID_PPV_ARGS(m_DxgiFactory.GetAddressOf()));
	if (FAILED(DXGIRes))
	{
		throw std::invalid_argument("SwapChain::Constructor() => Failed to create DXGIFactory");
	}

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	SwapChainDesc.Width = 0;
	SwapChainDesc.Height = 0;
	SwapChainDesc.Format = BackBufferFormat;
	SwapChainDesc.Stereo = false;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferCount = 3;
	SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapChainDesc.Flags = (DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	//scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY; // NOTE - Test it!

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc = {};
	DEVMODEA DisplayInfo;
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &DisplayInfo);

	FullscreenDesc.RefreshRate.Numerator = DisplayInfo.dmDisplayFrequency;
	FullscreenDesc.RefreshRate.Denominator = 1;
	FullscreenDesc.Windowed = true;
	FullscreenDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	FullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	const HRESULT SwapChainRes = m_DxgiFactory->CreateSwapChainForHwnd(GraphicsQueue.GetCommandQueue(), WindowHandle,
		&SwapChainDesc, &FullscreenDesc, nullptr, m_SwapChain.GetAddressOf());
	if (FAILED(SwapChainRes))
	{
		throw std::invalid_argument("SwapChain::Constructor() => Failed to create SwapChain");
	}

	this->SetBackbufferResources(OptResourceRecycler);
}

aZero::D3D12::SwapChain::~SwapChain()
{
	m_BackBuffers.clear();
}

void aZero::D3D12::SwapChain::Present()
{
	m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void aZero::D3D12::SwapChain::Resize(const DXM::Vector2& NewDimensions)
{
	D3D12_RESOURCE_DESC BBDesc = m_BackBuffers[0].GetResource()->GetDesc();
	m_BackBuffers.clear();

	// Recreate back buffers
	const HRESULT ResizeSwapChainRes = m_SwapChain->ResizeBuffers(
		3,
		NewDimensions.x,
		NewDimensions.y,
		BBDesc.Format,
		(DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
	);

	if (FAILED(ResizeSwapChainRes))
	{
		throw std::invalid_argument("SwapChain::Resize() => Failed to resize SwapChain backbuffers");
	}

	this->SetBackbufferResources(std::optional<D3D12::ResourceRecycler*>{});
}
