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

		// todo Make option to update by creating a version that is stored in a staging buffer and then copied using a command list
		class Descriptor : public NonCopyable
		{
			friend class DescriptorHeap;
		private:
			DescriptorHeap* m_diOwningHeap = nullptr;
			DescriptorIndex m_HeapIndex = Descriptor::InvalidDescriptorIndex;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;

			Descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, const D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle, const DescriptorIndex HeapIndex, DescriptorHeap* OwningHeap);
			void Move(Descriptor& other);

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
			ID3D12Device* const GetDevice() const;
		};

		class DescriptorHeap : public NonCopyable
		{
			friend class Descriptor;
		private:
			uint32_t m_DescriptorSize;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHeapStart;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHeapStart;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap = nullptr;
			aZero::DS::IndexFreelist<DescriptorIndex> m_Freelist;
			CallbackExecutor* m_diCallbackExecutor = nullptr;

			void OnDescriptorDestructor(Descriptor& descriptor);

		public:
			DescriptorHeap() = default;
			DescriptorHeap(ID3D12Device* device, CallbackExecutor& diCallbackExecutor, const D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, const bool gpuVisible);

			void Init(ID3D12Device* device, CallbackExecutor& diCallbackExecutor, const D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, const bool gpuVisible);

			Descriptor CreateDescriptor();
			void DestroyDescriptor(Descriptor&& descriptor);

			uint32_t GetMaxDescriptors() const;
			uint32_t GetDescriptorSize() const;

			D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;
			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuStart() const { return m_CpuHeapStart; }
			D3D12_GPU_DESCRIPTOR_HANDLE GetGpuStart() const { return m_GpuHeapStart; }

			ID3D12DescriptorHeap* Get() const { return m_Heap.Get(); }
			bool IsGpuVisible() const;
			bool IsInitiated() const { return m_Heap != nullptr; }
			ID3D12Device* const GetDevice() const;
		};
	}
}