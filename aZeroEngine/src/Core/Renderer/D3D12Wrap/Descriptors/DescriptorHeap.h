#pragma once

#include "Descriptor.h"
#include "Core/DataStructures/UniqueIndexList.h"

namespace aZero
{
	namespace D3D12
	{
		// TODO - Implement heap-auto-expand on GetDescriptor()
		class DescriptorHeap
		{
		private:
			bool m_gpuVisible = false;
			int m_descriptorSize = 0;
			Descriptor m_startDescriptor;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap = nullptr;
			aZero::DS::UniqueIndexList<uint32_t> m_IndexFreelist;

		public:
			DescriptorHeap() = default;

			DescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const int numDescriptors, const bool gpuVisible = false)
			{
				Initialize(device, type, numDescriptors, gpuVisible);
			}

			~DescriptorHeap()
			{

			}

			void Initialize(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const int numDescriptors, const bool gpuVisible = false)
			{
				if (!m_heap)
				{
					D3D12_DESCRIPTOR_HEAP_DESC desc;
					desc.NumDescriptors = numDescriptors;
					desc.Type = type;
					desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					desc.NodeMask = 0;

					if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
					{
						desc.Flags = gpuVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					}

					HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_heap.GetAddressOf()));
					if (FAILED(hr))
					{
						//TODO - Add debug fatal macro
						throw;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
					D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(0);
					if (desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
					{
						gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
						m_gpuVisible = true;
					}

					m_startDescriptor = Descriptor(cpuHandle, gpuHandle, 0);

					m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
				}
			}

			// TODO - Define operators etc...
			DescriptorHeap(const DescriptorHeap&) = delete;
			DescriptorHeap(DescriptorHeap&&) = delete;
			DescriptorHeap operator=(const DescriptorHeap&) = delete;
			DescriptorHeap operator=(DescriptorHeap&&) = delete;

			ID3D12DescriptorHeap* const GetDescriptorHeap() const { return m_heap.Get(); }

			Descriptor GetDescriptor()
			{
				const uint32_t descriptorIndex = m_IndexFreelist.GetIndex();

				if (descriptorIndex >= m_heap->GetDesc().NumDescriptors)
				{
					// TODO - Handle out-of-allocated-descriptors case
					throw;
				}
				
				D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = { 0 };
				cpuHandle.ptr = m_startDescriptor.GetCPUHandle().ptr + descriptorIndex * m_descriptorSize;

				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { 0 };
				if (m_gpuVisible)
				{
					gpuHandle.ptr = m_startDescriptor.GetGPUHandle().ptr + descriptorIndex * m_descriptorSize;
				}
			
				return Descriptor(cpuHandle, gpuHandle, descriptorIndex);
			}

			std::vector<Descriptor> GetDescriptors(const uint32_t numDescriptors)
			{
				// TODO - Try to avoid deep copy of vector
				std::vector<Descriptor> descriptors(numDescriptors);
				for (uint32_t i = 0; i < numDescriptors; i++)
				{
					descriptors[i] = GetDescriptor();
				}

				return descriptors;
			}
			
			void RecycleDescriptor(Descriptor& descriptor)
			{
				if (descriptor.GetHeapIndex() >= 0)
				{
					m_IndexFreelist.RecycleIndex(descriptor.GetHeapIndex());
					descriptor.m_heapIndex = 0;
				}
			}
		};
	}
}