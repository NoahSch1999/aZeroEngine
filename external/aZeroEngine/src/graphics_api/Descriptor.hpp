#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"

namespace aZero
{
	namespace D3D12
	{
		class Descriptor : public NonCopyable
		{
			friend class DescriptorHeap;
		private:
			int32_t m_HeapIndex = -1;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
			DescriptorHeap* m_OwningHeap = nullptr;

			Descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
				const D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
				const uint32_t HeapIndex)
				:m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle), m_HeapIndex(HeapIndex)
			{

			}

		public:
			Descriptor() = default;

			~Descriptor();

			Descriptor(Descriptor&& Other) noexcept
			{
				m_CpuHandle = Other.m_CpuHandle;
				m_GpuHandle = Other.m_GpuHandle;
				m_HeapIndex = Other.m_HeapIndex;
				m_OwningHeap = Other.m_OwningHeap;

				Other.m_OwningHeap = nullptr;
			}

			Descriptor& operator=(Descriptor&& Other) noexcept
			{
				if (this != &Other)
				{
					m_CpuHandle = Other.m_CpuHandle;
					m_GpuHandle = Other.m_GpuHandle;
					m_HeapIndex = Other.m_HeapIndex;
					m_OwningHeap = Other.m_OwningHeap;

					Other.m_OwningHeap = nullptr;
				}
				return *this;
			}

			D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_CpuHandle; }
			D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_GpuHandle; }
			uint32_t GetHeapIndex() const { return m_HeapIndex; }
		};
	}
}