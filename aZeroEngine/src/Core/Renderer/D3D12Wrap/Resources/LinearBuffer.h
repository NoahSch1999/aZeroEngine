#pragma once
#include "GPUResource.h"

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
				D3D12::GPUBuffer::RWPROPERTY RWProperty,
				uint64_t NumBytes,
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{})
			{
				this->Init(Device, RWProperty, NumBytes, OptResourceRecycler);
			}

			bool IsInitalized()
			{
				return m_Buffer.GetResource() != nullptr;
			}

			void Init(ID3D12Device* Device,
				D3D12::GPUBuffer::RWPROPERTY RWProperty,
				uint64_t NumBytes,
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{})
			{

				m_Buffer = std::move(D3D12::GPUBuffer(Device, RWProperty, NumBytes, OptResourceRecycler));
			}

			void Reset()
			{
				m_Offset = 0;
			}

			uint32_t GetOffset() const { return m_Offset; }

			void Write(void* Data, uint32_t NumBytes)
			{
				m_Buffer.Write(Data, NumBytes, m_Offset);

				if (m_Buffer.GetCPUAccessibleMemory())
				{
					m_Offset += NumBytes;
				}
			}

			D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const { return m_Buffer.GetResource()->GetGPUVirtualAddress(); }
		};
	}
}