#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include <optional>

#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace aZero
{
	class Engine;

	namespace D3D12
	{
		class ResourceRecycler;
		class CommandQueue;
	}

	namespace Window
	{
		// TODO: Impl waitable object logic
		class RenderWindow : public NonCopyable
		{
			friend Engine;
		private:
			Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain;
			Microsoft::WRL::ComPtr<IDXGIFactory5> m_Factory;

			HINSTANCE m_AppInstance;
			HWND m_WindowHandle;
			std::string m_Name;
			uint32_t m_NumBackBuffers;
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;

			D3D12::ResourceRecycler* m_ResourceRecycler = nullptr;

			void AllocateBackBuffers();

		public:

			// TODO: Add parenting:
				// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa#:~:text=in%2C%20optional%5D%20HWND-,hWndParent,-%2C%0A%20%20%5Bin%2C%20optional%5D%20HMENU
			RenderWindow(
				HINSTANCE AppInstance,
				const D3D12::CommandQueue& GraphicsQueue,
				const std::string& Name,
				const DXM::Vector2& WindowDimensions,
				const DXM::Vector2& BackBufferDimensions,
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{},
				std::uint32_t NumBackBuffers = 3,
				DXGI_FORMAT BackBufferFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

			~RenderWindow();

			RenderWindow(RenderWindow&& Other) noexcept
			{
				m_AppInstance = Other.m_AppInstance;
				m_WindowHandle = Other.m_WindowHandle;
				m_Name = std::move(Other.m_Name);
				m_SwapChain = Other.m_SwapChain;
				m_Factory = Other.m_Factory;
				m_NumBackBuffers = Other.m_NumBackBuffers;
				m_ResourceRecycler = Other.m_ResourceRecycler;
				m_BackBuffers = std::move(Other.m_BackBuffers);

				Other.m_ResourceRecycler = nullptr;
			}

			RenderWindow& operator&(RenderWindow&& Other) noexcept
			{
				if (this != &Other)
				{
					m_AppInstance = Other.m_AppInstance;
					m_WindowHandle = Other.m_WindowHandle;
					m_Name = std::move(Other.m_Name);
					m_SwapChain = Other.m_SwapChain;
					m_Factory = Other.m_Factory;
					m_NumBackBuffers = Other.m_NumBackBuffers;
					m_ResourceRecycler = Other.m_ResourceRecycler;
					m_BackBuffers = std::move(Other.m_BackBuffers);

					Other.m_ResourceRecycler = nullptr;
				}

				return *this;
			}

			ID3D12Resource* GetBackBuffer(uint32_t BufferIndex) 
			{
				const bool ValidIndex = BufferIndex < m_NumBackBuffers;
				if (!ValidIndex)
				{
					DEBUG_PRINT("Invalid index: " + std::to_string(BufferIndex) + "\nWindow has " + std::to_string(m_NumBackBuffers) + " back buffers");
					return nullptr;
				}
				return m_BackBuffers[BufferIndex].Get();
			}

			void Present()
			{
				m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
			}

			bool IsOpen()
			{
				HWND Handle = FindWindowA(m_Name.c_str(), m_Name.c_str());
				return IsWindow(m_WindowHandle);
				return Handle != NULL;
			}

			void HandleMessages()
			{
				MSG Msg = { 0 };
				while (PeekMessageA(&Msg, m_WindowHandle, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Msg);
					DispatchMessageA(&Msg);
				}
			}

			DXM::Vector2 GetDimensions() const
			{
				if (m_BackBuffers.size() > 0)
				{
					return DXM::Vector2(m_BackBuffers.at(0)->GetDesc().Width, m_BackBuffers.at(0)->GetDesc().Height);
				}
				return DXM::Vector2::Zero;
			}

			void Resize(const DXM::Vector2& NewDimensions)
			{
				// TODO: Impl
				throw;
			}

			void Show()
			{
				ShowWindow(m_WindowHandle, SW_SHOW);
			}

			void Hide()
			{
				ShowWindow(m_WindowHandle, SW_HIDE);
			}
		};
	}
}