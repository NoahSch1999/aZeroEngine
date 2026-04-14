#pragma once
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "graphics_api/command_recording/CommandList.hpp"

namespace aZero
{
	namespace Rendering
	{
		class LinearFrameAllocator : public NonCopyable
		{
		public:
			struct Allocation
			{
				RenderAPI::Buffer* DstResource = nullptr;
				uint32_t DstBufferOffset;
				uint32_t SrcBufferOffset;
				uint32_t NumBytes;
			};

		private:
			RenderAPI::Buffer m_StagingBuffer;
			uint32_t m_CurrentAllocOffset = 0;

			// TODO: Implement a more sophisticated approach without raw-pointers (both as hash and in the Allocation struct)
			std::unordered_map<RenderAPI::Buffer*, std::vector<Allocation>> m_Allocations;

			uint32_t GetNextAllocOffset(uint32_t NumBytes)
			{
				const uint32_t AllocOffset = m_CurrentAllocOffset;
				m_CurrentAllocOffset += NumBytes;
				return AllocOffset;
			}

			void Write(void* Data, uint32_t Offset, uint32_t NumBytes)
			{
				m_StagingBuffer.Write(Data, NumBytes, Offset);
			}

		public:
			LinearFrameAllocator() = default;

			LinearFrameAllocator(ID3D12DeviceX* device, uint32_t numBytes, RenderAPI::ResourceRecycler& recycler)
			{
				RenderAPI::Buffer::Desc desc(numBytes, D3D12_HEAP_TYPE_UPLOAD);
				m_StagingBuffer = RenderAPI::Buffer(device, desc, &recycler);
			}

			bool CanAllocate(uint32_t NumBytes)
			{
				const uint32_t OffsetAfterAlloc = m_CurrentAllocOffset + NumBytes;
				const uint32_t MemorySize = static_cast<uint32_t>(m_StagingBuffer.GetResource()->GetDesc().Width);
				return OffsetAfterAlloc <= MemorySize;
			}

			void AddAllocation(void* Data, RenderAPI::Buffer* DstResource, uint32_t DstOffset, uint32_t NumBytes)
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
					throw; // TODO: Handle
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

			void RecordAllocations(RenderAPI::CommandList& cmdList)
			{
				int OuterCounter = 0;
				for (const auto& [Resource, Allocations] : m_Allocations)
				{
					int InnerCounter = 0;
					for (const Allocation& Alloc : Allocations)
					{
						if (OuterCounter == 3 && InnerCounter == 1)
						{
							//continue;
						}
						cmdList->CopyBufferRegion(Alloc.DstResource->GetResource(), Alloc.DstBufferOffset, m_StagingBuffer.GetResource(), Alloc.SrcBufferOffset, Alloc.NumBytes);
						InnerCounter++;
					}

					OuterCounter++;
				}
			}
		};
	}
}