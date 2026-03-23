#pragma once
#include "misc/IndexFreelist.hpp"
#include "misc/NonCopyable.hpp"
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	class CallbackExecutor;

	namespace RenderAPI
	{
		using DescriptorIndex = uint32_t;

		class Descriptor : public NonCopyable
		{
			friend class DescriptorHeap;
		public:
			constexpr static DescriptorIndex InvalidDescriptorIndex = std::numeric_limits<DescriptorIndex>::max();

			Descriptor();
			~Descriptor();
			Descriptor(Descriptor&& other) noexcept;
			Descriptor& operator=(Descriptor&& other) noexcept;

			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_CpuHandle; }
			D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const { return m_GpuHandle; }
			DescriptorIndex GetHeapIndex() const { return m_HeapIndex; }
			bool IsValid() const { return m_HeapIndex != Descriptor::InvalidDescriptorIndex; }

		private:
			// TODO: Change so it's not a raw ptr. It needs to be valid if the heap is moved
			DescriptorHeap* m_diOwningHeap = nullptr;
			DescriptorIndex m_HeapIndex = Descriptor::InvalidDescriptorIndex;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;

			Descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, const D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle, const DescriptorIndex HeapIndex, DescriptorHeap* OwningHeap);
		};
	}
}