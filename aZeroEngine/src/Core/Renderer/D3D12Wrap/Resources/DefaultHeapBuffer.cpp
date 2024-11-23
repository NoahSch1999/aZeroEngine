#include "DefaultHeapBuffer.h"
#include "UploadHeapBuffer.h"

void aZero::D3D12::DefaultHeapBuffer::Write(uint32_t DstOffset, const UploadHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList)
{
	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, Src.GetResource(), SrcOffset, NumBytes);
}

void aZero::D3D12::DefaultHeapBuffer::Write(uint32_t DstOffset, void* Data, UploadHeapBuffer& SrcIntermediate, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList)
{
	SrcIntermediate.Write(SrcOffset, Data, NumBytes);
	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, SrcIntermediate.GetResource(), SrcOffset, NumBytes);
}

void aZero::D3D12::DefaultHeapBuffer::Write(uint32_t DstOffset, const DefaultHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList)
{
	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, Src.GetResource(), SrcOffset, NumBytes);
}