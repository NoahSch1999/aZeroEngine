#pragma once
#include "Core/Renderer/D3D12Wrap/Descriptors/Descriptor.h"
#include "Core/Renderer/D3D12Wrap/Resources/GPUTexture.h"

namespace aZero
{
	namespace Asset
	{
		struct LoadedTextureData
		{
			std::string FilePath;
			std::vector<D3D12_SUBRESOURCE_DATA> SubresourceData;
			std::unique_ptr<uint8_t[]> Data = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
		};

		bool LoadTexture2DFromFile(const std::string& FilePath, LoadedTextureData& OutData);

		class TextureFileAsset
		{
		private:
			std::string m_Name;
			D3D12::GPUTexture m_TextureResource;
		public:
			TextureFileAsset() = default;
			TextureFileAsset(const std::string& Name, const LoadedTextureData& Data, ID3D12GraphicsCommandList* CmdList)
			{
				this->Init(Name, Data, CmdList);
			}

			void Init(const std::string& Name, const LoadedTextureData& Data, ID3D12GraphicsCommandList* CmdList)
			{
				m_Name = Name;

				m_TextureResource.Init(Data.Resource, &gResourceRecycler);

				const UINT64 UploadBufferSize = GetRequiredIntermediateSize(Data.Resource.Get(), 0, static_cast<UINT>(Data.SubresourceData.size()));

				CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_UPLOAD);

				auto Desc = CD3DX12_RESOURCE_DESC::Buffer(UploadBufferSize);

				Microsoft::WRL::ComPtr<ID3D12Resource> StagingBuffer;
				HRESULT Hr = gDevice->CreateCommittedResource(
						&HeapProps,
						D3D12_HEAP_FLAG_NONE,
						&Desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(StagingBuffer.GetAddressOf()));

				if (FAILED(Hr))
				{
					throw;
				}

				UpdateSubresources(CmdList, Data.Resource.Get(), StagingBuffer.Get(), 0, 0, static_cast<UINT>(Data.SubresourceData.size()), Data.SubresourceData.data());

				gResourceRecycler.RecycleResource(StagingBuffer);
			}

			const D3D12::GPUTexture& GetResource() const { return m_TextureResource; }
		};
	}
}