#pragma once
#include <vector>
#include <functional>
#include "misc/IndexFreelist.hpp"
#include "misc/NonCopyable.hpp"
#include "misc/CallbackExecutor.hpp"
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		using DescriptorIndex = uint32_t;

		// TODO: Make option to update by creating a version that is stored in a staging buffer and then copied using a command list
		class Descriptor : public NonCopyable
		{
			friend class DescriptorHeap;
		private:
			DescriptorHeap* m_diOwningHeap = nullptr;
			DescriptorIndex m_HeapIndex = Descriptor::InvalidDescriptorIndex;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;

			Descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
				const D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
				const DescriptorIndex HeapIndex, DescriptorHeap* OwningHeap)
				:m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle), m_HeapIndex(HeapIndex), m_diOwningHeap(OwningHeap)
			{

			}

			void Move(Descriptor& other)
			{
				m_CpuHandle = other.m_CpuHandle;
				m_GpuHandle = other.m_GpuHandle;
				m_HeapIndex = other.m_HeapIndex;
				m_diOwningHeap = other.m_diOwningHeap;

				other.m_HeapIndex = Descriptor::InvalidDescriptorIndex;
				other.m_diOwningHeap = nullptr;
			}

		public:
			constexpr static DescriptorIndex InvalidDescriptorIndex = std::numeric_limits<DescriptorIndex>::max();

			Descriptor()
				:m_diOwningHeap(nullptr), m_HeapIndex(Descriptor::InvalidDescriptorIndex){ }

			~Descriptor()
			{
				if (m_diOwningHeap)
				{
					m_diOwningHeap->OnDescriptorDestructor(*this);
				}
			}

			Descriptor(Descriptor&& other) noexcept
			{
				this->Move(other);
			}

			Descriptor& operator=(Descriptor&& other) noexcept
			{
				if (this != &other)
				{
					this->Move(other);
				}
				return *this;
			}

			D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_CpuHandle; }
			D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_GpuHandle; }
			DescriptorIndex GetHeapIndex() const { return m_HeapIndex; }
		};

		class DescriptorHeap : public NonCopyable
		{
			friend class Descriptor;
		private:
			uint32_t m_DescriptorSize;
			D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapStart;
			D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHeapStart;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap = nullptr;
			aZero::DS::IndexFreelist<DescriptorIndex> m_Freelist;
			CallbackExecutor* m_diCallbackExecutor = nullptr;

			void Move(DescriptorHeap& Other)
			{
				m_DescriptorSize = Other.m_DescriptorSize;
				m_CPUHeapStart = Other.m_CPUHeapStart;
				m_GPUHeapStart = Other.m_GPUHeapStart;
				m_Heap = std::move(m_Heap);
				m_Freelist = std::move(Other.m_Freelist);
				m_diCallbackExecutor = std::move(Other.m_diCallbackExecutor);
				Other.m_Heap = nullptr;
			}

			void OnDescriptorDestructor(Descriptor& descriptor)
			{
				if (descriptor.m_diOwningHeap == this)
				{
					DescriptorIndex index = descriptor.m_HeapIndex;
					m_diCallbackExecutor->Append([this, index] {
						this->m_Freelist.Recycle(index);
						});
					descriptor.m_HeapIndex = Descriptor::InvalidDescriptorIndex;
				}
			}

		public:
			DescriptorHeap() = default;

			DescriptorHeap(DescriptorHeap&& Other) noexcept
			{
				this->Move(Other);
			}

			DescriptorHeap& operator=(DescriptorHeap&& Other) noexcept
			{
				if (this != &Other)
				{
					this->Move(Other);
				}
				return *this;
			}

			DescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, const bool gpuVisible)
			{
				this->Init(device, type, numDescriptors, gpuVisible);
			}

			void Init()
			{
				if (!m_Heap)
				{
					D3D12_DESCRIPTOR_HEAP_DESC desc;
					desc.NumDescriptors = numDescriptors;
					desc.Type = type;
					desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					desc.NodeMask = 0;

					if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
					{
						desc.Flags = GpuVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					}

					const HRESULT res = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_Heap.GetAddressOf()));
					if (FAILED(res))
					{
						throw std::invalid_argument("DescriptorHeap::Init() => Failed to create heap");
					}

					m_CPUHeapStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
					if (desc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
					{
						m_GPUHeapStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
					}

					m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
				}
			}

			Descriptor CreateDescriptor()
			{
				const DescriptorIndex descriptorIndex = m_Freelist.New();
				if (descriptorIndex >= m_Heap->GetDesc().NumDescriptors)
				{
					throw std::invalid_argument("DescriptorHeap::GetDescriptor() => Out of descriptors");
				}

				D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = { 0 };
				cpuHandle.ptr = m_CPUHeapStart.ptr + descriptorIndex * m_DescriptorSize;

				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { 0 };
				if (m_Heap->GetDesc().Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
				{
					gpuHandle.ptr = m_GPUHeapStart.ptr + descriptorIndex * m_DescriptorSize;
				}

				return Descriptor(cpuHandle, gpuHandle, descriptorIndex, this);
			}

			void DestroyDescriptor(Descriptor&& descriptor)
			{
				if (descriptor.m_diOwningHeap == this)
				{
					descriptor = Descriptor();
				}
			}

			ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }
		};
	}
}