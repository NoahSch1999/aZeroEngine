#pragma once
#include "GPUResource.hpp"

namespace aZero
{
	namespace D3D12
	{
		class LinearBuffer
		{
		private:
			D3D12::GPUBuffer m_Buffer;
			uint32_t m_Offset = 0;

		public:
			LinearBuffer() = default;
			LinearBuffer(ID3D12Device* Device,
				D3D12_HEAP_TYPE HeapType,
				uint64_t NumBytes,
				D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Init(Device, HeapType, NumBytes, ResourceRecycler);
			}

			bool IsInitalized()
			{
				return m_Buffer.GetResource() != nullptr;
			}

			void Init(ID3D12Device* Device,
				D3D12_HEAP_TYPE HeapType,
				uint64_t NumBytes,
				D3D12::ResourceRecycler& ResourceRecycler)
			{

				m_Buffer = std::move(D3D12::GPUBuffer(Device, HeapType, NumBytes, ResourceRecycler));
			}

			void Reset()
			{
				m_Offset = 0;
			}

			uint32_t GetOffset() const { return m_Offset; }

			void Write(ID3D12GraphicsCommandList* CmdList, void* Data, uint32_t NumBytes)
			{
				const uint32_t OffsetAfterAlloc = m_Offset + NumBytes;
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

				m_Buffer.Write(CmdList, Data, NumBytes, m_Offset);

				if (m_Buffer.GetCPUAccessibleMemory())
				{
					m_Offset += NumBytes;
				}
			}

			D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const { return m_Buffer.GetResource()->GetGPUVirtualAddress(); }
			const D3D12::GPUBuffer& GetBuffer() const { return m_Buffer; }
			D3D12::GPUBuffer& GetBuffer() { return m_Buffer; }
		};
	}
}