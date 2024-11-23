#pragma once
#include "GPUBuffer.h"

namespace aZero
{
	namespace D3D12
	{
		class UploadHeapBuffer;

		class DefaultHeapBuffer : public GPUBuffer
		{
		private:

		public:
			DefaultHeapBuffer() = default;

			DefaultHeapBuffer(uint32_t NumElements, uint32_t BytesPerElement, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(NumElements, BytesPerElement, Format, OptResourceRecycler);
			}

			void Init(uint32_t NumElements, uint32_t BytesPerElement, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				GPUBuffer::Init(NumElements, BytesPerElement, Format, D3D12_HEAP_TYPE_DEFAULT, OptResourceRecycler);
			}

			void Write(uint32_t DstOffset, void* Data, UploadHeapBuffer& SrcIntermediate, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList);
			void Write(uint32_t DstOffset, const UploadHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList);
			void Write(uint32_t DstOffset, const DefaultHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList);
		};
	}
}