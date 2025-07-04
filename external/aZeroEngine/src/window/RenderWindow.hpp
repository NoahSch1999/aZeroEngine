#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include <optional>

#include "graphics_api/D3D12Include.hpp"
#include "renderer/ResourceRecycler.hpp"
#include "graphics_api/resource_type/GPUResourceView.hpp"
#include "misc/NonCopyable.hpp"
#include "graphics_api/CommandQueue.hpp"

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
		// TODO: Add multi-window support for engine
		class RenderWindow : public NonCopyable
		{
			friend Engine;
		private:

			// TODO: Cleanup member vars
			Microsoft::WRL::ComPtr<IDXGISwapChain2> m_SwapChain;
			HANDLE m_WaitableHandle;

			Microsoft::WRL::ComPtr<IDXGIFactory2> m_Factory;

			HWND m_WindowHandle;
			std::string m_Name;
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;
			uint32_t m_NextBackBuffer = 0;

			D3D12::ResourceRecycler* m_ResourceRecycler = nullptr;

			void PopulateBackBuffers()
			{
				for (int BufferIndex = 0; BufferIndex < m_BackBuffers.size(); BufferIndex++)
				{
					Microsoft::WRL::ComPtr<ID3D12Resource> NewResource;
					m_SwapChain->GetBuffer(BufferIndex, IID_PPV_ARGS(m_BackBuffers[BufferIndex].GetAddressOf()));
				}

				m_NextBackBuffer = 0;
			}

			void ReleaseBuffers(bool Immediate = false)
			{
				if (m_BackBuffers.size() > 0)
				{
					for (Microsoft::WRL::ComPtr<ID3D12Resource>& BackBuffer : m_BackBuffers)
					{
						if (!Immediate)
						{
							m_ResourceRecycler->RecycleResource(BackBuffer);
						}
						BackBuffer = nullptr;
					}
				}
			}

			void CreateSwapchain(
				const D3D12::CommandQueue& GraphicsQueue,
				const DXM::Vector2& BackBufferDimensions,
				uint32_t NumBackBuffers, 
				DXGI_FORMAT Format);

			void MoveOp(RenderWindow& Other)
			{
				m_WindowHandle = Other.m_WindowHandle;
				m_Name = std::move(Other.m_Name);
				m_SwapChain = Other.m_SwapChain;
				m_Factory = Other.m_Factory;
				m_ResourceRecycler = Other.m_ResourceRecycler;
				m_BackBuffers = std::move(Other.m_BackBuffers);

				Other.m_ResourceRecycler = nullptr;
			}

		public:

			// TODO: Add parenting:
				// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa#:~:text=in%2C%20optional%5D%20HWND-,hWndParent,-%2C%0A%20%20%5Bin%2C%20optional%5D%20HMENU
			RenderWindow(
				const D3D12::CommandQueue& GraphicsQueue,
				const std::string& Name,
				const DXM::Vector2& WindowDimensions,
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{},
				std::uint32_t NumBackBuffers = 3,
				DXGI_FORMAT BackBufferFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

			~RenderWindow();

			RenderWindow(RenderWindow&& Other) noexcept
			{
				this->MoveOp(Other);
			}

			RenderWindow& operator=(RenderWindow&& Other) noexcept
			{
				if (this != &Other)
				{
					this->MoveOp(Other);
				}

				return *this;
			}

			bool WaitOnSwapchain()
			{
				return WaitForSingleObject(m_WaitableHandle, 0) == WAIT_OBJECT_0;
			}

			HWND GetHandle() const { return m_WindowHandle; }

			ID3D12Resource* GetBackCurrentBuffer() 
			{
				return m_BackBuffers.at(m_NextBackBuffer % m_BackBuffers.size()).Get();
			}

			void Present()
			{
				m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
				m_NextBackBuffer++;
			}

			// TODO: Impl so bb is always same as window client area
			void SetFullscreenMode(D3D12::CommandQueue& CmdQueue, bool Fullscreen)
			{
				if (Fullscreen)
				{
					SetWindowLongA(m_WindowHandle, GWL_STYLE, WS_POPUP);
					ShowWindow(m_WindowHandle, SW_SHOW);
					// Resize swapchain
					CmdQueue.FlushImmediate();
					int x = GetSystemMetrics(SM_CXSCREEN);
					int y = GetSystemMetrics(SM_CYSCREEN);
					this->Resize(DXM::Vector2(x, y));
				}
				else
				{
					SetWindowLongA(m_WindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);
					// Resize swapchain
					ShowWindow(m_WindowHandle, SW_SHOW); 
					CmdQueue.FlushImmediate();
					auto ClientDims = this->GetClientDimensions();
					this->Resize(ClientDims);
				}
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

			DXM::Vector2 GetClientDimensions() const
			{
				RECT Rect;
				GetClientRect(m_WindowHandle, &Rect);
				return DXM::Vector2(Rect.right - Rect.left, Rect.bottom - Rect.top);
			}

			void Resize(const DXM::Vector2& NewDimensions)
			{
				WINDOWPLACEMENT WndPl;
				GetWindowPlacement(m_WindowHandle, &WndPl);
				SetWindowPos(
					m_WindowHandle, 0, 
					WndPl.rcNormalPosition.left, WndPl.rcNormalPosition.bottom,
					NewDimensions.x, NewDimensions.y, 
					SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

				this->ReleaseBuffers(true);

				DXGI_SWAP_CHAIN_DESC SwapChainDesc;
				m_SwapChain->GetDesc(&SwapChainDesc);
				m_SwapChain->ResizeBuffers(
					m_BackBuffers.size(),
					NewDimensions.x, NewDimensions.y,
					SwapChainDesc.BufferDesc.Format,
					(DXGI_SWAP_CHAIN_FLAG)(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
					| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);

				this->PopulateBackBuffers();
			}
		};
	}
}