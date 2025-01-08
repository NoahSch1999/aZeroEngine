#pragma once
//#include "DefaultHeapBuffer.h"
#include "LinearFrameAllocator.h"
#include "Core/DataStructures/FreelistAllocator.h"


namespace aZero
{
	namespace D3D12
	{
		class FreelistBuffer
		{
		private:

			D3D12::GPUBuffer m_Buffer;
			DS::FreelistAllocator m_Allocator;

		public:
			FreelistBuffer() = default;

			void Init(ID3D12Device* Device, uint32_t NumBytes, std::optional<ResourceRecycler*> OptResourceRecycler, D3D12::GPUResource::RWPROPERTY RWProperty)
			{
				m_Buffer.Init(Device, RWProperty, NumBytes, OptResourceRecycler);
				m_Allocator.Init(NumBytes);
			}

			void Write(D3D12::LinearFrameAllocator& Allocator, const DS::FreelistAllocator::AllocationHandle& Handle, void* Data)
			{
				Allocator.AddAllocation(Data, &m_Buffer, Handle.GetStartOffset(), Handle.GetNumBytes());
			}

			void Write(D3D12::LinearFrameAllocator& Allocator, void* Data, uint32_t Offset, uint32_t NumBytes)
			{
				Allocator.AddAllocation(Data, &m_Buffer, Offset, NumBytes);
			}

			void GetAllocation(DS::FreelistAllocator::AllocationHandle& OutHandle, uint32_t NumBytes)
			{
				m_Allocator.Allocate(OutHandle, NumBytes);
			}

			const D3D12::GPUBuffer& GetBuffer() const { return m_Buffer; }
		};
	}
}