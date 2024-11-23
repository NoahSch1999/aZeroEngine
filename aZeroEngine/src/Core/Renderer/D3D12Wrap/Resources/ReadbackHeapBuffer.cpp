//#include "ReadbackHeapBuffer.h"
//#include "UploadHeapBuffer.h"
//#include "DefaultHeapBuffer.h"
//
//void aZero::D3D12::ReadbackHeapBuffer::Write(int DstOffset, const UploadHeapBuffer& Src, int SrcOffset, int NumBytes, ID3D12GraphicsCommandList* CmdList)
//{
//	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, Src.GetResource(), SrcOffset, NumBytes);
//}
//
//void aZero::D3D12::ReadbackHeapBuffer::Write(int DstOffset, const DefaultHeapBuffer& Src, int SrcOffset, int NumBytes, ID3D12GraphicsCommandList* CmdList)
//{
//	CmdList->CopyBufferRegion(this->GetResource(), (UINT64)DstOffset, Src.GetResource(), SrcOffset, NumBytes);
//}
//
//void aZero::D3D12::ReadbackHeapBuffer::Readback(int DstOffset, void* DataPtr, int NumBytes, int SrcOffset)
//{
//	memcpy((char*)DataPtr + DstOffset, (char*)GetMappedPtr() + SrcOffset, NumBytes);
//}
