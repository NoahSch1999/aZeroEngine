#include "SwapChain.h"

aZero::D3D12::SwapChain::SwapChain(HWND windowHandle, const CommandQueue& graphicsQueue, DXGI_FORMAT backBufferFormat)
{
	if (FAILED(graphicsQueue.GetCommandQueue()->GetDevice(IID_PPV_ARGS(&gDevice))))
	{
		throw;
	}

	if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(m_dxgiFactory.GetAddressOf()))))
	{
		throw;
	}

	DXGI_SWAP_CHAIN_DESC1 scDesc;
	scDesc.Width = 0;
	scDesc.Height = 0;
	scDesc.Format = backBufferFormat;
	scDesc.Stereo = false;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferCount = 3;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	scDesc.Flags = (DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
	//scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY; // NOTE - Test it!

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
	DEVMODEA displayInfo;
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &displayInfo);

	fullscreenDesc.RefreshRate.Numerator = displayInfo.dmDisplayFrequency;
	fullscreenDesc.RefreshRate.Denominator = 1;
	fullscreenDesc.Windowed = true;
	fullscreenDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	if (FAILED(m_dxgiFactory->CreateSwapChainForHwnd(graphicsQueue.GetCommandQueue(), windowHandle,
		&scDesc, &fullscreenDesc, nullptr, m_swapChain.GetAddressOf())))
	{
		throw;
	}

	m_backBuffers.resize(3);

	for (int i = 0; i < 3; i++)
	{
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backBuffers[i].GetAddressOf()));
	}
}

aZero::D3D12::SwapChain::~SwapChain()
{
	for (Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer : m_backBuffers)
	{
		if (backBuffer)
		{
			gResourceRecycler.RecycleResource(backBuffer);
			backBuffer = nullptr;
		}
	}
}

//void aZero::D3D12::SwapChain::ResolveRenderSurface(ID3D12GraphicsCommandList* cmdList, int backBufferIndex, D3D12::GPUResource& renderSource)
//{
//	if (backBufferIndex > m_backBuffers.size())
//	{
//		throw;
//	}
//
//	// NOTE - input texture should be same resolution as the backbuffer. So the renderer has to upscale/downscale it to match...
//
//	ID3D12Resource* backBuffer = m_backBuffers[backBufferIndex].Get();
//	D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();
//	D3D12_RESOURCE_DESC descs = renderSource.GetResource()->GetDesc();
//
//	D3D12::GPUResource::TransitionState(cmdList, backBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
//	D3D12::GPUResource::TransitionState(cmdList, renderSource, D3D12_RESOURCE_STATE_COPY_SOURCE);
//
//	// NOTE - Copying causes the resources to go decay into D3D12_RESOURCE_STATE_COMMON once the GPU has executed the recorded command ???? OR NOT ????
//	cmdList->CopyResource(backBuffer, renderSource.GetResource());
//
//	D3D12::GPUResource::TransitionState(cmdList, backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
//}

void aZero::D3D12::SwapChain::Present()
{
	m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

void aZero::D3D12::SwapChain::Resize(const DXM::Vector2& NewDimensions)
{
	D3D12_RESOURCE_DESC BBDesc = m_backBuffers[0]->GetDesc();
	for (Microsoft::WRL::ComPtr<ID3D12Resource>& Buffer : m_backBuffers)
	{
		Buffer->Release();
	}

	// Recreate back buffers
	if (FAILED(m_swapChain->ResizeBuffers(
		m_backBuffers.size(),
		NewDimensions.x,
		NewDimensions.y,
		BBDesc.Format,
		(DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
	)))
	{
		throw;
	}

	for (int i = 0; i < 3; i++)
	{
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backBuffers[i].GetAddressOf()));
	}
}
