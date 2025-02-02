#pragma once
#include "GPUResource.hpp"
#include "graphics_api/CommandContext.hpp"

namespace aZero
{
	namespace D3D12
	{
		class LinearFrameAllocator : public NonCopyable
		{
		public:
			struct Allocation
			{
				D3D12::GPUBuffer* DstResource = nullptr;
				uint32_t DstBufferOffset;
				uint32_t SrcBufferOffset;
				uint32_t NumBytes;
			};

		private:
			D3D12::CommandContext m_CommandContext;
			D3D12::GPUBuffer m_StagingBuffer;
			uint32_t m_CurrentAllocOffset = 0;

			// TODO: Implement a more sophisticated approach without raw-pointers (both as hash and in the Allocation struct)
			std::unordered_map<D3D12::GPUBuffer*, std::vector<Allocation>> m_Allocations;

			uint32_t GetNextAllocOffset(uint32_t NumBytes)
			{
				const uint32_t AllocOffset = m_CurrentAllocOffset;
				m_CurrentAllocOffset += NumBytes;
				return AllocOffset;
			}

			void Write(ID3D12GraphicsCommandList* CmdList, void* Data, uint32_t Offset, uint32_t NumBytes)
			{
				m_StagingBuffer.Write(CmdList, Data, NumBytes, Offset);
			}

		public:
			LinearFrameAllocator() = default;

			LinearFrameAllocator(LinearFrameAllocator&& Other) noexcept
			{
				m_CommandContext = std::move(Other.m_CommandContext);
				m_Allocations = std::move(Other.m_Allocations);
				m_StagingBuffer = std::move(Other.m_StagingBuffer);
				m_CurrentAllocOffset = Other.m_CurrentAllocOffset;
			}

			LinearFrameAllocator& operator=(LinearFrameAllocator&& Other) noexcept
			{
				if (this != &Other)
				{
					m_CommandContext = std::move(Other.m_CommandContext);
					m_Allocations = std::move(Other.m_Allocations);
					m_StagingBuffer = std::move(Other.m_StagingBuffer);
					m_CurrentAllocOffset = Other.m_CurrentAllocOffset;
				}
				return *this;
			}

			LinearFrameAllocator(ID3D12Device* Device, uint32_t NumBytes, D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Init(Device, NumBytes, ResourceRecycler);
			}

			void Init(ID3D12Device* Device, uint32_t NumBytes, D3D12::ResourceRecycler& ResourceRecycler)
			{
				m_CommandContext.Init(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
				m_StagingBuffer.Init(Device, D3D12_HEAP_TYPE_UPLOAD, NumBytes, ResourceRecycler);
			}

			bool CanAllocate(uint32_t NumBytes)
			{
				const uint32_t OffsetAfterAlloc = m_CurrentAllocOffset + NumBytes;
				const uint32_t MemorySize = static_cast<uint32_t>(m_StagingBuffer.GetResource()->GetDesc().Width);
				return OffsetAfterAlloc <= MemorySize;
			}

			void AddAllocation(ID3D12GraphicsCommandList* CmdList, void* Data, D3D12::GPUBuffer* DstResource, uint32_t DstOffset, uint32_t NumBytes)
			{
				const UINT64 DstResourceSize = DstResource->GetResource()->GetDesc().Width;
				if (DstResourceSize < (DstOffset + NumBytes))
				{
					throw std::invalid_argument("LinearFrameAllocator::AddAllocation() => OOR write to DstResource");
				}

				const uint32_t OffsetAfterAlloc = m_CurrentAllocOffset + NumBytes;
				const uint32_t MemorySizeBytes = static_cast<uint32_t>(m_StagingBuffer.GetResource()->GetDesc().Width);
				if (OffsetAfterAlloc > MemorySizeBytes)
				{
					uint32_t NewSize = MemorySizeBytes * 2;
					if (OffsetAfterAlloc > NewSize)
					{
						NewSize = OffsetAfterAlloc;
					}

					this->m_StagingBuffer.Resize(CmdList, NewSize);
				}

				const uint32_t SrcAllocOffset = this->GetNextAllocOffset(NumBytes);
				this->Write(CmdList, Data, SrcAllocOffset, NumBytes);

				Allocation Alloc;
				Alloc.DstBufferOffset = DstOffset;
				Alloc.DstResource = DstResource;
				Alloc.NumBytes = NumBytes;
				Alloc.SrcBufferOffset = SrcAllocOffset;

				m_Allocations[DstResource].push_back(Alloc);
			}

			void Reset()
			{
				m_Allocations.clear();
				m_CurrentAllocOffset = 0;
			}

			void RecordAllocations(ID3D12GraphicsCommandList* CmdList)
			{
				int OuterCounter = 0;
				for (const auto& [Resource, Allocations] : m_Allocations)
				{
					int InnerCounter = 0;
					for (const D3D12::LinearFrameAllocator::Allocation& Alloc : Allocations)
					{
						if (OuterCounter == 3 && InnerCounter == 1)
						{
							//continue;
						}
						CmdList->CopyBufferRegion(Alloc.DstResource->GetResource(), Alloc.DstBufferOffset, m_StagingBuffer.GetResource(), Alloc.SrcBufferOffset, Alloc.NumBytes);
						InnerCounter++;
					}

					OuterCounter++;
				}
			}

			D3D12::CommandContext& GetCommandContext() { return m_CommandContext; }
		};
	}
}