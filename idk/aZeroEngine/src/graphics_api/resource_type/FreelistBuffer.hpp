#pragma once
#include "LinearFrameAllocator.hpp"
#include "misc/FreelistAllocator.hpp"


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

			void Init(ID3D12Device* Device, uint32_t NumBytes, D3D12::ResourceRecycler& ResourceRecycler, D3D12_HEAP_TYPE HeapType)
			{
				m_Buffer.Init(Device, HeapType, NumBytes, ResourceRecycler);
				m_Allocator.Init(NumBytes);
			}

			void Write(ID3D12GraphicsCommandList* CmdList, D3D12::LinearFrameAllocator& Allocator, const DS::FreelistAllocator::AllocationHandle& Handle, void* Data)
			{
				const uint32_t OffsetAfterAlloc = Handle.GetStartOffset() + Handle.GetNumBytes();
				const uint32_t MemorySizeBytes = static_cast<uint32_t>(m_Buffer.GetResource()->GetDesc().Width);
				if (OffsetAfterAlloc > MemorySizeBytes)
				{
					uint32_t NewSize = MemorySizeBytes * 2;
					if (OffsetAfterAlloc > NewSize)
					{
						NewSize = OffsetAfterAlloc;
					}

					m_Buffer.Resize(CmdList, NewSize);
				}

				Allocator.AddAllocation(CmdList, Data, &m_Buffer, Handle.GetStartOffset(), Handle.GetNumBytes());
			}

			void Write(ID3D12GraphicsCommandList* CmdList, D3D12::LinearFrameAllocator& Allocator, void* Data, uint32_t Offset, uint32_t NumBytes)
			{
				const uint32_t OffsetAfterAlloc = Offset + NumBytes;
				const uint32_t MemorySizeBytes = static_cast<uint32_t>(m_Buffer.GetResource()->GetDesc().Width);
				if (OffsetAfterAlloc > MemorySizeBytes)
				{
					uint32_t NewSize = MemorySizeBytes * 2;
					if (OffsetAfterAlloc > NewSize)
					{
						NewSize = OffsetAfterAlloc;
					}

					m_Buffer.Resize(CmdList, NewSize);
				}
				Allocator.AddAllocation(CmdList, Data, &m_Buffer, Offset, NumBytes);
			}

			void Allocate(DS::FreelistAllocator::AllocationHandle& OutHandle, uint32_t NumBytes)
			{
				m_Allocator.Allocate(OutHandle, NumBytes);
			}

			const D3D12::GPUBuffer& GetBuffer() const { return m_Buffer; }
			D3D12::GPUBuffer& GetBuffer() { return m_Buffer; }
		};
	}
}