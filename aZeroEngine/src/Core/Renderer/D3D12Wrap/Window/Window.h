#pragma once
#include <Windows.h>
#include <string>
#include <memory>

#include "SwapChain.h"
#include "Core/Renderer/D3D12Wrap/Commands/CommandQueue.h"
#include "Core/Renderer/D3D12Wrap/Resources/ResourceRecycler.h"
#include "Core/Renderer/D3D12Wrap/Resources/GPUResource.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace aZero
{
	class Window
	{
	private:
		std::unique_ptr<D3D12::SwapChain> m_swapChain = nullptr;
		HWND m_windowHandle;
		std::wstring m_windowName;
		HINSTANCE m_appInstance;
		DXM::Vector2 m_lastWindowedDimensions = { 0, 0 };
		DXGI_FORMAT m_renderSurfaceFormat;

	public:
		Window(const D3D12::CommandQueue& graphicsQueue,
			HINSTANCE appInstance,
			WNDPROC winProcedure,
			DXGI_FORMAT backBufferFormat,
			const DXM::Vector2& dimensions, bool fullscreen, const std::string& windowName);

		~Window();

		void Init(const D3D12::CommandQueue& graphicsQueue, HINSTANCE appInstance,
			WNDPROC winProcedure,
			DXGI_FORMAT backBufferFormat,
			const DXM::Vector2& dimensions, bool fullscreen, const std::string& windowName);

		bool IsOpen();
		void Resize(const DXM::Vector2& dimensions, const DXM::Vector2& position = { 0, 0 });
		void SetFullscreen(bool enabled);
		DXM::Vector2 GetClientDimensions() const;
		DXM::Vector2 GetFullDimensions() const;

		D3D12::SwapChain& GetSwapChain() { return *m_swapChain.get(); }

		void HandleMessages();

		Window(const Window&) = delete;
		Window(Window&&) = delete;
		Window operator=(const Window&) = delete;
		Window operator=(Window&&) = delete;
	};
}

