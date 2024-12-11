#pragma once
#include "Core/D3D12Include.h"
#include "Core/Renderer/D3D12Wrap/Descriptors/Descriptor.h"
#include "Core/Renderer/D3D12Wrap/Commands/CommandQueue.h"
#include "Core/Renderer/D3D12Wrap/Resources/GPUResource.h"

namespace aZero
{
	namespace D3D12
	{
		class SwapChain
		{
		private:
			Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain = nullptr; // NOTE - Change to SwapChain3
			Microsoft::WRL::ComPtr<IDXGIFactory5> m_DxgiFactory = nullptr;

			std::vector<D3D12::GPUTexture> m_BackBuffers;

			void SetBackbufferResources(std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				D3D12::ResourceRecycler* ResourceRecycler = OptResourceRecycler.has_value() ? OptResourceRecycler.value() : nullptr;
				if (m_BackBuffers.size() > 0)
				{
					ResourceRecycler = m_BackBuffers[0].GetResourceRecycler();
				}

				m_BackBuffers.resize(3);

				for (int i = 0; i < 3; i++)
				{
					Microsoft::WRL::ComPtr<ID3D12Resource> NewResource;
					m_SwapChain->GetBuffer(i, IID_PPV_ARGS(NewResource.GetAddressOf()));
					m_BackBuffers[i] = std::move(D3D12::GPUTexture(NewResource, ResourceRecycler));
				}
			}

		public:
			SwapChain(HWND WindowHandle, const CommandQueue& GraphicsQueue, DXGI_FORMAT BackBufferFormat, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler);
				
			// NOTE - Critical to flush the GPU commands using a CPU side wait before the SwapChain gets destroyed!
			~SwapChain();

			void Present();

			void Resize(const DXM::Vector2& NewDimensions);

			ID3D12Resource* GetBackBufferResource(int index) { return m_BackBuffers[index].GetResource(); }
		};
	}
}