#pragma once
#include "UploadHeapBuffer.h"
#include "Core/Renderer/D3D12Wrap/Commands/CommandContext.h"

namespace aZero
{
	namespace D3D12
	{
		class FrameAllocator
		{
		public:
			struct Allocation
			{
				D3D12::GPUBuffer* DstResource = nullptr;
				int DstBufferOffset;
				int SrcBufferOffset;
				int NumBytes;
			};
		private:
			D3D12::CommandContext m_CommandContext;
			std::vector<Allocation> m_Allocations;
			D3D12::UploadHeapBuffer m_StagingBuffer;
			int m_CurrentAllocOffset = 0;

			int GetNextAllocOffset(int NumBytes)
			{
				const int AllocOffset = m_CurrentAllocOffset;
				m_CurrentAllocOffset += NumBytes;
				return AllocOffset;
			}

		public:
			FrameAllocator() = default;

			FrameAllocator(D3D12::CommandContext CommandContext, UINT NumBytes)
			{
				this->Init(CommandContext, NumBytes);
			}

			void Init(D3D12::CommandContext CommandContext, UINT NumBytes)
			{
				m_CommandContext = CommandContext;
				m_StagingBuffer.Init(1, NumBytes, DXGI_FORMAT_UNKNOWN, &gResourceRecycler);
			}

			void AddAllocation(void* Data, D3D12::GPUBuffer* DstResource, int DstOffset, int NumBytes)
			{
				const int SrcAllocOffset = this->GetNextAllocOffset(NumBytes);
				Allocation Alloc;
				Alloc.DstBufferOffset = DstOffset;
				Alloc.DstResource = DstResource;
				Alloc.NumBytes = NumBytes;
				Alloc.SrcBufferOffset = SrcAllocOffset;
				m_Allocations.push_back(Alloc);

				this->Write(Data, SrcAllocOffset, NumBytes);
			}

			void Reset()
			{
				m_Allocations.resize(0);
				m_CurrentAllocOffset = 0;
			}

			void Write(void* Data, UINT Offset, UINT NumBytes)
			{
				UINT AllocatedSizeBytes = m_StagingBuffer.GetResource()->GetDesc().Width;
				if (Offset + NumBytes > AllocatedSizeBytes)
				{
					// OOR
					throw;
				}
				memcpy((char*)m_StagingBuffer.GetMappedPtr() + Offset, Data, NumBytes);
			}

			void RecordAllocations()
			{
				ID3D12GraphicsCommandList* const CmdList = m_CommandContext.GetCommandList();
				for (const D3D12::FrameAllocator::Allocation& Alloc : m_Allocations)
				{
					CmdList->CopyBufferRegion(Alloc.DstResource->GetResource(), Alloc.DstBufferOffset, m_StagingBuffer.GetResource(), Alloc.SrcBufferOffset, Alloc.NumBytes);
				}
			}

			ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandContext.GetCommandList(); }
		};
	}
}