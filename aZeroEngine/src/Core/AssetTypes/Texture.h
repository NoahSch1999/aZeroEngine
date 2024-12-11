#pragma once
#include "Core/D3D12Include.h"
#include "Core/AssetTypes/RenderFileAsset.h"
#include "Core/Renderer/D3D12Wrap/Resources/GPUResourceView.h"

namespace aZero
{
	namespace Asset
	{
		struct TextureData
		{
			ID3D12Resource* TempResource;
			/*std::vector<uint8_t> m_RawData;
			DXM::Vector2 Dimensions;*/
			// TODO: Should contain everything needed to create a texture with mipmaps etc
		};

		struct TextureGPUHandle
		{
			D3D12::GPUTexture m_Texture;
			D3D12::ShaderResourceView m_SRV;
		};

		class Texture : public RenderFileAsset<TextureData, TextureGPUHandle>
		{
		private:

		public:
			uint32_t m_DescriptorIndex = 0;
			Texture() = default;

			~Texture()
			{
				if (this->HasRenderState())
				{
					if (m_AssetGPUHandle.use_count() == 1)
					{
						this->RemoveRenderState();
					}
				}
			}

			virtual bool HasRenderState() const override
			{
				if (m_AssetGPUHandle->m_Texture.GetResource())
				{
					return true;
				}

				return false;
			}

			virtual bool LoadFromFile(const std::string& Path) override
			{
				if (true)
				{
					//TextureData NewTexture;
					if (/*LoadFBXFile(NewMesh, Path)*/true)
					{
						//this->SetAssetData(std::move(NewTexture));
					}
				}
				return true;
			}

			uint32_t GetDescriptorIndex() const { return m_DescriptorIndex; }
		};
	}
}