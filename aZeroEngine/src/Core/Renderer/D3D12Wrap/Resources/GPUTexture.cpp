#include "GPUTexture.h"
#include "UploadHeapBuffer.h"

void aZero::D3D12::GPUTexture::Write(
	const std::vector<D3D12_SUBRESOURCE_DATA>& Data,
	int FirstSubresource,
	int NumSubresources,
	const UploadHeapBuffer& Src,
	int SrcOffset,
	ID3D12GraphicsCommandList* CmdList
)
{
	UpdateSubresources(CmdList, this->GetResource(), Src.GetResource(),
		SrcOffset, FirstSubresource, NumSubresources, Data.data());
}
