#pragma once
#include "graphics_api/SwapChain.hpp"

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace aZero;

struct WindowWrapper
{
	RenderAPI::SwapChain m_SwapChain;

	std::string m_Name;
	HANDLE m_WaitableHandle;
	HWND m_WindowHandle;
	Rendering::Renderer* di_Renderer = nullptr;

	WindowWrapper() = default;

	WindowWrapper(
		Rendering::Renderer& renderer,
		const std::string& name,
		const DXM::Vector2& windowDimensions,
		const DXM::Vector2& backBufferDimensions
	)
	{
		this->Init(renderer, name, windowDimensions, backBufferDimensions);
	}

	void Init(
		Rendering::Renderer& renderer,
		const std::string& name, 
		const DXM::Vector2& windowDimensions,
		const DXM::Vector2& backBufferDimensions)
	{
		m_Name = name;
		di_Renderer = &renderer;

		WNDCLASSA wc = { };
		ZeroMemory(&wc, sizeof(wc));
		wc.lpfnWndProc = WndProc;

		auto moduleHandle = GetModuleHandleA(nullptr);
		wc.hInstance = moduleHandle;
		wc.lpszClassName = m_Name.c_str();
		wc.style = CS_CLASSDC;

		const ATOM registerClassRes = RegisterClassA(&wc);
		if (!registerClassRes)
		{
			const DWORD errorCode = GetLastError();
			DEBUG_PRINT("RegisterClass had error code: " + std::to_string(errorCode));
			throw std::invalid_argument("Failed to register window");
		}

		m_WindowHandle = CreateWindowExA(
			0,
			m_Name.c_str(),
			m_Name.c_str(),
			WS_OVERLAPPEDWINDOW,
			0, 0,
			windowDimensions.x, windowDimensions.y,
			NULL,
			NULL,
			moduleHandle,
			NULL
		);

		if (m_WindowHandle == NULL)
		{
			const DWORD errorCode = GetLastError();
			DEBUG_PRINT("CreateWindow had error code: " + (int32_t)errorCode);
			throw std::invalid_argument("Failed to create window");
		}

		ShowWindow(m_WindowHandle, SW_SHOW);
		UpdateWindow(m_WindowHandle);

		m_SwapChain.Init(m_WindowHandle, renderer.GetGraphicsCommandQueue(), backBufferDimensions, renderer.GetBufferingCount(), DXGI_FORMAT_R8G8B8A8_UNORM);

		UINT latency = 2;
		m_SwapChain.Get()->GetMaximumFrameLatency(&latency);
		m_WaitableHandle = m_SwapChain.Get()->GetFrameLatencyWaitableObject();
	}

	void HandleMessages()
	{
		MSG msg = { 0 };
		while (PeekMessageA(&msg, m_WindowHandle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	DXM::Vector2 GetClientDimensions() const
	{
		RECT rect;
		GetClientRect(m_WindowHandle, &rect);
		return DXM::Vector2(static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top));
	}

	void Resize(const DXM::Vector2& newDimensions)
	{
		WINDOWPLACEMENT wndPl;
		GetWindowPlacement(m_WindowHandle, &wndPl);
		SetWindowPos(
			m_WindowHandle, 0,
			wndPl.rcNormalPosition.left, wndPl.rcNormalPosition.bottom,
			static_cast<UINT>(newDimensions.x), static_cast<UINT>(newDimensions.y),
			SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		m_SwapChain.Resize(di_Renderer->GetGraphicsCommandQueue(), newDimensions);
	}

	bool WaitOnSwapchain() { return WaitForSingleObject(m_WaitableHandle, 0) == WAIT_OBJECT_0; }
	bool IsOpen() { return IsWindow(m_WindowHandle); }
	void Present() { m_SwapChain.Present(); }
};

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}