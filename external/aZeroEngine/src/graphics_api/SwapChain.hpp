#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "graphics_api/command_recording/CommandQueue.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class SwapChain : public NonCopyable
		{
		public:
			SwapChain() = default;
			SwapChain(HWND wndHandle, RenderAPI::CommandQueue& cmdQueue, const DXM::Vector2& backBufferDimensions, uint32_t numBackBuffers, DXGI_FORMAT format);
			
			SwapChain(SwapChain&& other) noexcept;
			SwapChain& operator=(SwapChain&& other) noexcept;
			
			~SwapChain();

			IDXGISwapChainX* Get() { return m_SwapChain.Get(); }
			ID3D12Resource* GetFrameBackBuffer() { return m_BackBuffers.at(m_NextBackBuffer % m_BackBuffers.size()).Get(); }
			void Present() { m_diCmdQueue->Signal(); m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING); m_NextBackBuffer++; }
			void Resize(RenderAPI::CommandQueue& cmdQueue, const DXM::Vector2& backBufferDimensions);
		private:
			void PopulateBackBuffers();
			void ReleaseBuffers() { m_BackBuffers.clear(); }

			Microsoft::WRL::ComPtr<IDXGISwapChainX> m_SwapChain;
			Microsoft::WRL::ComPtr<IDXGIFactoryX> m_Factory;
			RenderAPI::CommandQueue* m_diCmdQueue = nullptr;
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;
			uint32_t m_NextBackBuffer = 0;
		};
	}
}