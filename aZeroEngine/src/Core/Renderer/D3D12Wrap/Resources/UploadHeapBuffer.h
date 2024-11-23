#pragma once
#include "GPUBuffer.h"

namespace aZero
{
	namespace D3D12
	{
		class DefaultHeapBuffer;

		class UploadHeapBuffer : public GPUBuffer
		{
		private:

		public:
			UploadHeapBuffer() = default;

			UploadHeapBuffer(uint32_t NumElements, uint32_t BytesPerElement, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(NumElements, BytesPerElement, Format, OptResourceRecycler);
			}

			void Init(uint32_t NumElements, uint32_t BytesPerElement, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				GPUBuffer::Init(NumElements, BytesPerElement, Format, D3D12_HEAP_TYPE_UPLOAD, OptResourceRecycler);
			}

			void Write(uint32_t DstOffset, void* Data, uint32_t NumBytes);
			void Write(uint32_t DstOffset, const UploadHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList);
			void Write(uint32_t DstOffset, const DefaultHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList);
		};
	}
}