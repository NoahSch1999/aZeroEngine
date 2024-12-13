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
			std::vector<uint8_t> m_Data;
			DXM::Vector2 m_Dimensions;
			DXGI_FORMAT m_Format;
		};

		bool LoadTextureFromFile(const std::string& Path, TextureData& OutData);

		struct TextureGPUHandle
		{
			D3D12::GPUTexture m_Texture;
			D3D12::ShaderResourceView m_SRV;
		};

		class Texture : public RenderFileAsset<TextureData, TextureGPUHandle>
		{
		public:
			Texture() = default;

			~Texture()
			{
				if (this->HasRenderState())
				{
					this->RemoveRenderState();
				}
			}

			virtual bool HasRenderState() const override
			{
				if (m_AssetGPUHandle.m_Texture.GetResource())
				{
					return true;
				}

				return false;
			}

			virtual bool LoadFromFile(const std::string& Path) override
			{
				TextureData NewTexture;
				if (LoadTextureFromFile(Path, NewTexture))
				{
					this->SetAssetData(std::move(NewTexture));
				}
				else
				{
					return false;
				}

				return true;
			}

			uint32_t GetDescriptorIndex() const { return m_AssetGPUHandle.m_SRV.GetDescriptorIndex(); }
		};
	}
}