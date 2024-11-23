#pragma once
#include "Core/D3D12Core.h"
#include "Core/Renderer/D3D12Wrap/Descriptors/Descriptor.h"
#include "Core/Renderer/D3D12Wrap/Commands/CommandQueue.h"
#include "Core/Renderer/D3D12Wrap/Resources/GPUResource.h"

namespace aZero
{
	namespace D3D12
	{
		// Renderer should create and own descriptors when the window is registered
		class SwapChain
		{
		private:
			Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain = nullptr; // NOTE - Change to SwapChain3
			Microsoft::WRL::ComPtr<IDXGIFactory5> m_dxgiFactory = nullptr;

			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;

		public:
			SwapChain(HWND windowHandle,
				const CommandQueue& graphicsQueue,
				DXGI_FORMAT backBufferFormat);
				

			// NOTE - Critical to flush the GPU commands using a CPU side wait before the SwapChain gets destroyed!
			~SwapChain();

			void Present();

			void Resize(const DXM::Vector2& NewDimensions);

			ID3D12Resource* GetBackBufferResource(int index) { return m_backBuffers[index].Get(); }

			SwapChain(const SwapChain&) = delete;
			SwapChain(SwapChain&&) = delete;
			SwapChain operator=(const SwapChain&) = delete;
			SwapChain operator=(SwapChain&&) = delete;
		};
	}
}