#pragma once
//#include "UploadHeapBuffer.h"
#include "GPUResource.h"
#include "Core/Renderer/D3D12Wrap/Commands/CommandContext.h"

namespace aZero
{
	namespace D3D12
	{
		class LinearFrameAllocator
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

			void Write(void* Data, uint32_t Offset, uint32_t NumBytes)
			{
				memcpy((char*)m_StagingBuffer.GetCPUAccessibleMemory() + Offset, Data, NumBytes);
			}

		public:
			LinearFrameAllocator() = default;

			LinearFrameAllocator(const LinearFrameAllocator&) = delete;
			LinearFrameAllocator& operator=(const LinearFrameAllocator&) = delete;

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

			LinearFrameAllocator(ID3D12Device* Device, uint32_t NumBytes)
			{
				this->Init(Device, NumBytes);
			}

			void Init(ID3D12Device* Device, uint32_t NumBytes)
			{
				m_CommandContext.Init(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
				m_StagingBuffer.Init(Device, D3D12::GPUResource::RWPROPERTY::CPUWRITE, NumBytes, std::optional<D3D12::ResourceRecycler*>{});
			}

			bool CanAllocate(uint32_t NumBytes)
			{
				const uint32_t OffsetAfterAlloc = m_CurrentAllocOffset + NumBytes;
				const uint32_t MemorySize = static_cast<uint32_t>(m_StagingBuffer.GetResource()->GetDesc().Width);
				return OffsetAfterAlloc <= MemorySize;
			}

			void AddAllocation(void* Data, D3D12::GPUBuffer* DstResource, uint32_t DstOffset, uint32_t NumBytes)
			{
				const UINT64 DstResourceSize = DstResource->GetResource()->GetDesc().Width;
				if (DstResourceSize < (DstOffset + NumBytes))
				{
					throw std::invalid_argument("LinearFrameAllocator::AddAllocation() => Out-of-range write to DstResource");
				}

				// TODO: Expand staging buffer if needed
				if (!this->CanAllocate(NumBytes))
				{
					throw std::invalid_argument("LinearFrameAllocator::AddAllocation() => Out-of-range write to this");
				}

				const uint32_t SrcAllocOffset = this->GetNextAllocOffset(NumBytes);
				this->Write(Data, SrcAllocOffset, NumBytes);

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

			void RecordAllocations()
			{
				for (const auto& [Resource, Allocations] : m_Allocations)
				{
					for (const D3D12::LinearFrameAllocator::Allocation& Alloc : Allocations)
					{
						m_CommandContext.GetCommandList()->CopyBufferRegion(Alloc.DstResource->GetResource(), Alloc.DstBufferOffset, m_StagingBuffer.GetResource(), Alloc.SrcBufferOffset, Alloc.NumBytes);
					}
				}
			}

			D3D12::CommandContext& GetCommandContext() { return m_CommandContext; }
		};
	}
}