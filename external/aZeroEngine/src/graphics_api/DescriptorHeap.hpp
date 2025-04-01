#pragma once
#include <set>

#include "Descriptor.hpp"

namespace aZero
{
	namespace D3D12
	{
		// TODO: Implement heap-auto-expand on GetDescriptor()
		class DescriptorHeap : public NonCopyable
		{
		private:
			uint32_t m_DescriptorSize;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapStart;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHeapStart;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap = nullptr;

			std::set<uint32_t> m_ReusedIndices;
			uint32_t m_NextFreeIndex = 0;

			uint32_t GetNextIndex()
			{
				uint32_t NextIndex;
				if (!m_ReusedIndices.empty())
				{
					NextIndex = *m_ReusedIndices.begin();
					m_ReusedIndices.erase(NextIndex);
				}

				NextIndex = m_NextFreeIndex;
				m_NextFreeIndex++;
				return NextIndex;
			}

			void RecycleIndex(uint32_t ToRecycle)
			{
				if (ToRecycle < m_NextFreeIndex)
				{
					m_ReusedIndices.insert(ToRecycle);
				}
			}

		public:
			DescriptorHeap() = default;

			DescriptorHeap(DescriptorHeap&& Other) noexcept
			{
				m_DescriptorSize = Other.m_DescriptorSize;
				m_CPUHeapStart = Other.m_CPUHeapStart;
				m_GPUHeapStart = Other.m_GPUHeapStart;
				m_Heap = std::move(m_Heap);
				m_ReusedIndices = std::move(m_ReusedIndices);
				m_NextFreeIndex = Other.m_NextFreeIndex;
			}

			DescriptorHeap& operator=(DescriptorHeap&& Other) noexcept
			{
				if (this != &Other)
				{
					m_DescriptorSize = Other.m_DescriptorSize;
					m_CPUHeapStart = Other.m_CPUHeapStart;
					m_GPUHeapStart = Other.m_GPUHeapStart;
					m_Heap = std::move(m_Heap);
					m_ReusedIndices = std::move(m_ReusedIndices);
					m_NextFreeIndex = Other.m_NextFreeIndex;
				}
				return *this;
			}

			DescriptorHeap(ID3D12Device* const Device, const D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptors, const bool GpuVisible = false)
			{
				this->Init(Device, Type, NumDescriptors, GpuVisible);
			}

			void Init(ID3D12Device* const Device, const D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptors, const bool GpuVisible = false)
			{
				if (!m_Heap)
				{
					D3D12_DESCRIPTOR_HEAP_DESC Desc;
					Desc.NumDescriptors = NumDescriptors;
					Desc.Type = Type;
					Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					Desc.NodeMask = 0;

					if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
					{
						Desc.Flags = GpuVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					}

					const HRESULT Res = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(m_Heap.GetAddressOf()));
					if (FAILED(Res))
					{
						throw std::invalid_argument("DescriptorHeap::Init() => Failed to create heap");
					}

					m_CPUHeapStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
					if (Desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
					{
						m_GPUHeapStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
					}

					m_DescriptorSize = Device->GetDescriptorHandleIncrementSize(Type);
				}
			}

			Descriptor GetDescriptor()
			{
				const uint32_t DescriptorIndex = this->GetNextIndex();
				if (DescriptorIndex >= m_Heap->GetDesc().NumDescriptors)
				{
					throw std::invalid_argument("DescriptorHeap::GetDescriptor() => Out of descriptors");
				}
				
				D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = { 0 };
				CpuHandle.ptr = m_CPUHeapStart.ptr + DescriptorIndex * m_DescriptorSize;

				D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = { 0 };
				if (m_Heap->GetDesc().Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
				{
					GpuHandle.ptr = m_GPUHeapStart.ptr + DescriptorIndex * m_DescriptorSize;
				}
			
				return Descriptor(CpuHandle, GpuHandle, DescriptorIndex, this);
			}
			
			void RecycleDescriptor(Descriptor& Descriptor)
			{
				if (Descriptor.m_OwningHeap == this)
				{
					this->RecycleIndex(Descriptor.GetHeapIndex());
					Descriptor.m_OwningHeap = nullptr;
				}
			}

			// Unsafe version for ImGui
			void RecycleDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
			{
				const uint32_t CPUIndex = (m_CPUHeapStart.ptr - CPUHandle.ptr) / m_DescriptorSize;
				this->RecycleIndex(CPUIndex);
			}

			ID3D12DescriptorHeap* const GetDescriptorHeap() const { return m_Heap.Get(); }
		};
	}
}