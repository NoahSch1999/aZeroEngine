#pragma once
#include "DescriptorHeap.h"

namespace aZero
{
	namespace D3D12
	{
		class DescriptorManager
		{
		private:
			DescriptorHeap m_ResourceHeap;
			DescriptorHeap m_SamplerHeap;
			DescriptorHeap m_RtvHeap;
			DescriptorHeap m_DsvHeap;

		public:
			DescriptorManager() = default;

			DescriptorManager(ID3D12Device* device)
			{
				Initialize(device);
			}

			~DescriptorManager()
			{

			}

			// TODO - Add heap expansion and remove hardcodeded values
			void Initialize(ID3D12Device* device)
			{
				if (!m_ResourceHeap.GetDescriptorHeap())
				{
					m_ResourceHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100000, true);
					m_SamplerHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
					m_RtvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
					m_DsvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);
				}
			}

			// TODO - Define copy stuff etc...
			DescriptorManager(const DescriptorManager&) = delete;
			DescriptorManager(DescriptorManager&&) = delete;
			DescriptorManager operator=(const DescriptorManager&) = delete;
			DescriptorManager operator=(DescriptorManager&&) = delete;

			DescriptorHeap& GetResourceHeap() { return m_ResourceHeap; }
			DescriptorHeap& GetSamplerHeap() { return m_SamplerHeap; }
			DescriptorHeap& GetRtvHeap() { return m_RtvHeap; }
			DescriptorHeap& GetDsvHeap() { return m_DsvHeap; }
		};
	}
}