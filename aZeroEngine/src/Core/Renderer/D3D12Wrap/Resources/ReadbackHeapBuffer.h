//#pragma once
//#include "GPUBuffer.h"

/* IN PROGRESS */

//namespace aZero
//{
//	namespace D3D12
//	{
//		class DefaultHeapBuffer;
//		class UploadHeapBuffer;
//
//		class ReadbackHeapBuffer : public GPUBuffer
//		{
//		private:
//
//		public:
//			ReadbackHeapBuffer() = default;
//
//			ReadbackHeapBuffer(int NumElements, int BytesPerElement, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
//			{
//				this->Init(NumElements, BytesPerElement, Format, OptResourceRecycler);
//			}
//
//			void Init(int NumElements, int BytesPerElement, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
//			{
//				GPUBuffer::Init(NumElements, BytesPerElement, Format, D3D12_HEAP_TYPE_READBACK, OptResourceRecycler);
//			}
//
//			void Write(int DstOffset, const UploadHeapBuffer& Src, int SrcOffset, int NumBytes, ID3D12GraphicsCommandList* CmdList);
//			void Write(int DstOffset, const DefaultHeapBuffer& Src, int SrcOffset, int NumBytes, ID3D12GraphicsCommandList* CmdList);
//
//			void Readback(int DstOffset, void* DataPtr, int NumBytes, int SrcOffset);
//		};
//	}
//}