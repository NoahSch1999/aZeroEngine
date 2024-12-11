#include "Window.h"
#include "SwapChain.h"

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

aZero::Window::Window::Window(const D3D12::CommandQueue& graphicsQueue, HINSTANCE appInstance,
	WNDPROC winProcedure, DXGI_FORMAT backBufferFormat, const DXM::Vector2& dimensions, bool fullscreen, const std::string& windowName)
{
	Init(graphicsQueue, appInstance, winProcedure, backBufferFormat, dimensions, fullscreen, windowName);
}

void aZero::Window::Init(const D3D12::CommandQueue& graphicsQueue, HINSTANCE appInstance,
	WNDPROC winProcedure, DXGI_FORMAT backBufferFormat, const DXM::Vector2& dimensions, bool fullscreen, const std::string& windowName)
{
	m_renderSurfaceFormat = backBufferFormat;
	m_lastWindowedDimensions = dimensions;
	m_appInstance = appInstance;
	m_windowName.assign(windowName.begin(), windowName.end()); // TODO - possible to get name via handle for unreg?

	WNDCLASS wc = { };
	wc.lpfnWndProc = winProcedure;
	wc.hInstance = m_appInstance;
	wc.lpszClassName = m_windowName.c_str();
	RegisterClass(&wc);

	m_windowHandle = CreateWindowEx(
		0,
		m_windowName.c_str(),
		m_windowName.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		dimensions.x, dimensions.y,
		NULL,
		NULL,
		m_appInstance,
		NULL
	);

	if (m_windowHandle == NULL)
	{
		throw std::invalid_argument("Window::Init() => Failed to create window");
	}

	if (fullscreen)
	{
		SetFullscreen(true);
	}

	ShowWindow(m_windowHandle, SW_SHOWNORMAL);

	m_swapChain = std::make_unique<D3D12::SwapChain>(m_windowHandle, graphicsQueue, backBufferFormat, std::optional<D3D12::ResourceRecycler*>{});
}

aZero::Window::Window::~Window()
{
	DestroyWindow(m_windowHandle);
	UnregisterClass(m_windowName.c_str(), m_appInstance);
}

void aZero::Window::Window::SetFullscreen(bool enabled)
{
	if (enabled)
	{
		m_lastWindowedDimensions = GetFullDimensions();

		SetWindowLongPtr(m_windowHandle, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowLongPtr(m_windowHandle, GWL_EXSTYLE, WS_EX_APPWINDOW);

		const HMONITOR monitor = MonitorFromWindow(m_windowHandle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);
		GetMonitorInfoW(monitor, &monitorInfo);

		SetWindowPos(m_windowHandle, nullptr,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_NOZORDER);
	}
	else
	{
		SetWindowLongPtr(m_windowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowLongPtr(m_windowHandle, GWL_EXSTYLE, WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW);
		SetWindowPos(m_windowHandle, NULL, 0, 0, m_lastWindowedDimensions.x, m_lastWindowedDimensions.y, 0);
	}
}

bool aZero::Window::Window::IsOpen()
{
	HWND handle = FindWindow(m_windowName.c_str(), m_windowName.c_str());
	return handle != NULL;
}

void aZero::Window::Window::Resize(const DXM::Vector2& dimensions, const DXM::Vector2& position)
{
	m_lastWindowedDimensions = dimensions;
	SetWindowPos(m_windowHandle, NULL, position.x, position.y, dimensions.x, dimensions.y, 0);

	m_swapChain->Resize(dimensions);
}

DXM::Vector2 aZero::Window::Window::GetClientDimensions() const
{
	RECT rect;
	GetClientRect(m_windowHandle, &rect);
	return DXM::Vector2(rect.right, rect.bottom);
}

DXM::Vector2 aZero::Window::Window::GetFullDimensions() const
{
	RECT rect;
	GetWindowRect(m_windowHandle, &rect);
	return DXM::Vector2(rect.right, rect.bottom);
}

void aZero::Window::Window::HandleMessages()
{
	MSG msg = { 0 };
	bool msgReturn = 1;
	while (PeekMessage(&msg, m_windowHandle, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
