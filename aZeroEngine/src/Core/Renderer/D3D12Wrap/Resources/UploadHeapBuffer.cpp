#include "UploadHeapBuffer.h"
#include "DefaultHeapBuffer.h"

void aZero::D3D12::UploadHeapBuffer::Write(uint32_t DstOffset, void* Data, uint32_t NumBytes)
{
	memcpy((char*)this->GetMappedPtr() + DstOffset, Data, NumBytes);
}

void aZero::D3D12::UploadHeapBuffer::Write(uint32_t DstOffset, const UploadHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList)
{
	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, Src.GetResource(), SrcOffset, NumBytes);
}

void aZero::D3D12::UploadHeapBuffer::Write(uint32_t DstOffset, const DefaultHeapBuffer& Src, uint32_t SrcOffset, uint32_t NumBytes, ID3D12GraphicsCommandList* CmdList)
{
	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, Src.GetResource(), SrcOffset, NumBytes);
}
