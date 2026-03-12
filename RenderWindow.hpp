#pragma once
#include "include/window/SDLWindow_Win32.hpp"
#include "graphics_api/SwapChain.hpp"
#include "renderer/Renderer.hpp"

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class RenderWindow : public aZero::Window::SDLWindow_Win32 {
public:
	RenderWindow() = default;
	explicit RenderWindow(const aZero::Window::SDLWindowDesc& desc, aZero::Rendering::Renderer& renderer)
		:SDLWindow_Win32(desc), di_Renderer(&renderer) 
	{
		m_SwapChain.Init(this->GetNativeHandle(), renderer.GetGraphicsCommandQueue(), { static_cast<float>(desc.rect.w), static_cast<float>(desc.rect.h) }, renderer.GetBufferingCount(), DXGI_FORMAT_R8G8B8A8_UNORM);

		UINT latency = 2;
		m_SwapChain.Get()->GetMaximumFrameLatency(&latency);
		m_WaitableHandle = m_SwapChain.Get()->GetFrameLatencyWaitableObject();
	}

	bool WaitOnSwapchain() { return WaitForSingleObject(m_WaitableHandle, 0) == WAIT_OBJECT_0; }
	void Present() { m_SwapChain.Present(); }
	void Update() { 
		this->PollEvents();
	}

private:
	void PollEventImpl(const SDL_Event& event) final {
		if (event.key.type == SDL_EVENT_KEY_DOWN) {
			if (event.key.key == SDLK_ESCAPE) {
				this->Close(); // Close window
			}
		}
	}

	aZero::RenderAPI::SwapChain m_SwapChain;
	HANDLE m_WaitableHandle;
	aZero::Rendering::Renderer* di_Renderer = nullptr;
};