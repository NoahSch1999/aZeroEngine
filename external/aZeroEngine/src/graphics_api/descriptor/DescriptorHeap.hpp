#pragma once
#include "Descriptor.hpp"

namespace aZero
{
	class CallbackExecutor;

	namespace RenderAPI
	{
		// TODO: Add resize functionality
		class DescriptorHeap : public NonCopyable
		{
			friend class Descriptor;
		public:
			DescriptorHeap() = default;
			DescriptorHeap(ID3D12DeviceX* device, CallbackExecutor& diCallbackExecutor, const D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, const bool gpuVisible);

			// Note - All created descriptors need to have their destructor called before the instance is moved
			// TODO: Change this since its very bad designed and will lead to annoying crashes
			DescriptorHeap(DescriptorHeap&& other) noexcept;
			DescriptorHeap& operator=(DescriptorHeap&& other) noexcept;

			Descriptor CreateDescriptor();

			uint32_t GetMaxDescriptors() const;
			uint32_t GetDescriptorSize() const;

			D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;
			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuStart() const { return m_CpuHeapStart; }
			D3D12_GPU_DESCRIPTOR_HANDLE GetGpuStart() const { return m_GpuHeapStart; }

			ID3D12DescriptorHeap* Get() const { return m_Heap.Get(); }
			bool IsGpuVisible() const;
			bool IsInitiated() const { return m_Heap != nullptr; }
			ID3D12DeviceX* GetDevice() const;

		private:
			uint32_t m_DescriptorSize;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHeapStart;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHeapStart;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap = nullptr;
			aZero::DS::IndexFreelist<DescriptorIndex> m_Freelist;
			CallbackExecutor* m_diCallbackExecutor = nullptr;

			void OnDescriptorDestructor(Descriptor& descriptor);
		};
	}
}