#include "RenderWindow.hpp"

#include "graphics_api/resource_type/GPUResource.hpp"
#include "graphics_api/CommandQueue.hpp"

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void aZero::Window::RenderWindow::CreateSwapchain(
	const D3D12::CommandQueue& GraphicsQueue,
	const DXM::Vector2& BackBufferDimensions,
	uint32_t NumBackBuffers,
	DXGI_FORMAT Format)
{

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	SwapChainDesc.Width = BackBufferDimensions.x;
	SwapChainDesc.Height = BackBufferDimensions.y;
	SwapChainDesc.Format = Format;
	SwapChainDesc.Stereo = false;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferCount = NumBackBuffers;
	SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapChainDesc.Flags =
		(DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
		| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc = {};
	DEVMODEA DisplayInfo;
	EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &DisplayInfo);

	FullscreenDesc.RefreshRate.Numerator = DisplayInfo.dmDisplayFrequency;
	FullscreenDesc.RefreshRate.Denominator = 1;
	FullscreenDesc.Windowed = true;
	FullscreenDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	FullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	const HRESULT SwapChainRes = m_Factory->CreateSwapChainForHwnd(GraphicsQueue.GetCommandQueue(), m_WindowHandle,
		&SwapChainDesc, &FullscreenDesc, nullptr, (IDXGISwapChain1**)m_SwapChain.GetAddressOf());
	if (FAILED(SwapChainRes))
	{
		throw std::invalid_argument("Failed to create swapchain");
	}

	m_SwapChain->SetMaximumFrameLatency(2);
	m_WaitableHandle = m_SwapChain->GetFrameLatencyWaitableObject();

	this->ReleaseBuffers();

	m_BackBuffers.resize(m_NumBackBuffers);

	this->PopulateBackBuffers();
}

aZero::Window::RenderWindow::RenderWindow(
	HINSTANCE AppInstance,
	const D3D12::CommandQueue& GraphicsQueue,
	const std::string& Name,
	const DXM::Vector2& WindowDimensions,
	const DXM::Vector2& BackBufferDimensions,
	std::optional<D3D12::ResourceRecycler*> OptResourceRecycler,
	std::uint32_t NumBackBuffers,
	DXGI_FORMAT BackBufferFormat)
{
	m_AppInstance = AppInstance;
	m_Name.assign(Name.begin(), Name.end());
	m_NumBackBuffers = NumBackBuffers;
	m_ResourceRecycler = OptResourceRecycler.has_value() ? OptResourceRecycler.value() : nullptr;

	WNDCLASSA wc = { };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = AppInstance;
	wc.lpszClassName = m_Name.c_str();

	std::wstring X(Name.begin(), Name.end());
	
	const ATOM RegisterClassRes = RegisterClassA(&wc);
	if (!RegisterClassRes)
	{
		const DWORD ErrorCode = GetLastError();
		DEBUG_PRINT("RegisterClass had error code: " + std::to_string(ErrorCode));
		throw std::invalid_argument("Failed to register window");
	}

	m_WindowHandle = CreateWindowExA(
		0,
		m_Name.c_str(),
		m_Name.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WindowDimensions.x, WindowDimensions.y,
		NULL,
		NULL,
		AppInstance,
		NULL
	);

	if (m_WindowHandle == NULL)
	{
		const DWORD ErrorCode = GetLastError();
		DEBUG_PRINT("CreateWindow had error code: " + (int32_t)ErrorCode);
		throw std::invalid_argument("Failed to create window");
	}

	ShowWindow(m_WindowHandle, SW_SHOWNORMAL);

	ID3D12Device* Device;
	const HRESULT GetDeviceRes = GraphicsQueue.GetCommandQueue()->GetDevice(IID_PPV_ARGS(&Device));
	if (FAILED(GetDeviceRes))
	{
		throw std::invalid_argument("Failed to get CommandQueue device");
	}

	const HRESULT DXGIRes = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_Factory.GetAddressOf()));
	if (FAILED(DXGIRes))
	{
		throw std::invalid_argument("Failed to create DXGIFactory");
	}

	this->CreateSwapchain(GraphicsQueue, BackBufferDimensions, NumBackBuffers, BackBufferFormat);
}

aZero::Window::RenderWindow::~RenderWindow()
{
	if (m_ResourceRecycler)
	{
		DestroyWindow(m_WindowHandle);
		UnregisterClassA(m_Name.c_str(), m_AppInstance);

		for (Microsoft::WRL::ComPtr<ID3D12Resource>& BackBuffer : m_BackBuffers)
		{
			m_ResourceRecycler->RecycleResource(BackBuffer);
		}
	}
}