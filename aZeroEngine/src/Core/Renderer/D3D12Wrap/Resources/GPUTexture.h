#pragma once
#include "GPUResource.h"

namespace aZero
{
	namespace D3D12
	{
		class DefaultHeapBuffer;
		class UploadHeapBuffer;

		class GPUTexture : public GPUResourceBase
		{
		public:
			GPUTexture() = default;

			GPUTexture(
				DXM::Vector3 Dimensions, 
				DXGI_FORMAT Format, 
				D3D12_RESOURCE_FLAGS Flags, 
				int MipLevels, 
				D3D12_RESOURCE_STATES InitialResourceState, 
				std::optional<D3D12_CLEAR_VALUE> OptOptimizedClearValue, 
				std::optional<ResourceRecycler*> OptResourceRecycler
			)
			{
				this->Init(Dimensions, Format, Flags, MipLevels, InitialResourceState, OptOptimizedClearValue, OptResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource>  ExistingResource, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				GPUResourceBase::Init(ExistingResource, OptResourceRecycler);
			}

			void Init(DXM::Vector3 Dimensions,
				DXGI_FORMAT Format,
				D3D12_RESOURCE_FLAGS Flags,
				int MipLevels,
				D3D12_RESOURCE_STATES InitialResourceState,
				std::optional<D3D12_CLEAR_VALUE> OptOptimizedClearValue,
				std::optional<ResourceRecycler*> OptResourceRecycler
			)
			{
				D3D12_RESOURCE_DESC ResourceDesc;
				ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));

				if (Dimensions.y > 0)
				{
					if (Dimensions.z > 1)
					{
						ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D;
					}
					else
					{
						ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					}
				}
				else
				{
					ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D;
				}

				ResourceDesc.Format = Format;
				ResourceDesc.Alignment = 0;
				ResourceDesc.Width = Dimensions.x;
				ResourceDesc.Height = Dimensions.y;
				ResourceDesc.DepthOrArraySize = Dimensions.z;
				ResourceDesc.MipLevels = MipLevels;
				ResourceDesc.SampleDesc.Count = 1;
				ResourceDesc.SampleDesc.Quality = 0;
				ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
				ResourceDesc.Flags = Flags;

				D3D12_HEAP_PROPERTIES HeapProperties;
				ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
				HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

				GPUResourceBase::Init(ResourceDesc, HeapProperties, D3D12_HEAP_FLAG_NONE, InitialResourceState, OptOptimizedClearValue, OptResourceRecycler);
			}

			void Write(
				const DXM::Vector3& DstOffset, 
				int DstMipLevel, 
				const DefaultHeapBuffer& Src, 
				int SrcOffset, 
				int NumBytes,
				ID3D12GraphicsCommandList* CmdList
			)
			{

			}

			// NOTE - Handle mips
			void Write(
				const std::vector<D3D12_SUBRESOURCE_DATA>& Data,
				int FirstSrcSubresource,
				int NumSubresources,
				const UploadHeapBuffer& Src,
				int SrcOffset,
				ID3D12GraphicsCommandList* CmdList
			);

			// NOTE - Temp not working
			void Write(
				const DXM::Vector3& DstOffset, 
				int DstSubResourceIndex, 
				const GPUTexture& Src, 
				int SrcSubResourceIndex,
				const D3D12_BOX SrcCopyBox,
				int NumBytes, 
				ID3D12GraphicsCommandList* CmdList
			)
			{
				D3D12_RESOURCE_DESC SrcDesc = Src.GetResource()->GetDesc();
				D3D12_RESOURCE_DESC DstDesc = this->GetResource()->GetDesc();

				if (SrcDesc.Dimension == DstDesc.Dimension
					&& SrcDesc.MipLevels == DstDesc.MipLevels
					&& SrcDesc.Alignment == DstDesc.Alignment
					&& SrcDesc.DepthOrArraySize == DstDesc.DepthOrArraySize
					&& SrcDesc.Width == DstDesc.Width
					&& SrcDesc.Height == DstDesc.Height
					&& SrcDesc.Format == DstDesc.Format
					&& SrcDesc.Layout == DstDesc.Layout
					&& SrcDesc.SampleDesc.Count == DstDesc.SampleDesc.Count
					&& SrcDesc.SampleDesc.Quality == DstDesc.SampleDesc.Quality
					)
				{
					CmdList->CopyResource(this->GetResource(), Src.GetResource());
					return;
				}

				D3D12_TEXTURE_COPY_LOCATION DstCopyLocation;
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT DstLayouts;
				UINT DstNumRows;
				UINT64 DstRowSizeInBytes;
				UINT64 DstTotalBytes;
				gDevice->GetCopyableFootprints(&DstDesc, 0, DstDesc.MipLevels, 0, &DstLayouts, &DstNumRows, &DstRowSizeInBytes, &DstTotalBytes);

				DstCopyLocation.pResource = this->GetResource();
				DstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				DstCopyLocation.SubresourceIndex = DstSubResourceIndex;
				DstCopyLocation.PlacedFootprint = DstLayouts;

				D3D12_TEXTURE_COPY_LOCATION SrcCopyLocation;
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT SrcLayouts;
				UINT SrcNumRows;
				UINT64 SrcRowSizeInBytes;
				UINT64 SrcTotalBytes;
				gDevice->GetCopyableFootprints(&SrcDesc, 0, SrcDesc.MipLevels, 0, &SrcLayouts, &SrcNumRows, &SrcRowSizeInBytes, &SrcTotalBytes);
				
				SrcCopyLocation.pResource = Src.GetResource();
				SrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				SrcCopyLocation.SubresourceIndex = SrcSubResourceIndex;
				SrcCopyLocation.PlacedFootprint = SrcLayouts;

				CmdList->CopyTextureRegion(&DstCopyLocation, DstOffset.x, DstOffset.y, DstOffset.z, &SrcCopyLocation, &SrcCopyBox);
			}
		};
	}
}