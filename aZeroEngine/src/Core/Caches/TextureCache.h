#pragma once
#include "Core/AssetTypes/TextureAsset.h"

namespace aZero
{
	class TextureFileCache
	{
	private:
		D3D12::DescriptorManager* m_DescriptorManager = nullptr;
		std::unordered_map<std::string, Asset::TextureFileAsset> m_Assets;
		std::unordered_map<std::string, D3D12::Descriptor> m_Descriptors;
	public:
		TextureFileCache() = default;

		TextureFileCache(D3D12::DescriptorManager& DescriptorManager)
		{
			this->Init(DescriptorManager);
		}

		void Init(D3D12::DescriptorManager& DescriptorManager)
		{
			m_DescriptorManager = &DescriptorManager;
		}

		void LoadFromFile(const std::string& FilePath, ID3D12GraphicsCommandList* CmdList)
		{
			const int LastSlashOffset = FilePath.find_last_of('/') + 1;
			const int LastDotOffset = FilePath.find_last_of('.');
			const int FileNameLength = (FilePath.length() - LastSlashOffset) - (FilePath.length() - LastDotOffset);
			const std::string FileName = FilePath.substr(LastSlashOffset, FileNameLength);

			Asset::LoadedTextureData LoadedTextureData;
			if (Asset::LoadTexture2DFromFile(FilePath, LoadedTextureData))
			{
				if (m_Assets.count(FileName) > 0)
				{
					// Handle duplicate texture-names
					throw;
					//
				}

				m_Assets.emplace(FileName, Asset::TextureFileAsset(FileName, LoadedTextureData, CmdList));

				D3D12_SHADER_RESOURCE_VIEW_DESC TextureSRVDesc;
				TextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				TextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				TextureSRVDesc.Texture2D.MipLevels = 1;
				TextureSRVDesc.Texture2D.MostDetailedMip = 0;
				TextureSRVDesc.Texture2D.PlaneSlice = 0;
				TextureSRVDesc.Texture2D.ResourceMinLODClamp = 0.f;
				TextureSRVDesc.Format = this->GetAsset(FileName)->GetResource().GetResource()->GetDesc().Format;
				m_Descriptors.emplace(FileName, m_DescriptorManager->GetResourceHeap().GetDescriptor());
				gDevice->CreateShaderResourceView(this->GetAsset(FileName)->GetResource().GetResource(), &TextureSRVDesc, m_Descriptors.at(FileName).GetCPUHandle());
			}
		}

		const Asset::TextureFileAsset* GetAsset(const std::string& Name)
		{
			if (m_Assets.count(Name) > 0)
			{
				return &m_Assets.at(Name);
			}

			return nullptr;
		}

		const D3D12::Descriptor* GetAssetDescriptor(const std::string& Name)
		{
			if (m_Descriptors.count(Name) > 0)
			{
				return &m_Descriptors.at(Name);
			}

			return nullptr;
		}

		void RemoveAsset(const std::string& Name)
		{
			if (m_Assets.count(Name) > 0)
			{
				m_Assets.erase(Name);
				m_DescriptorManager->GetResourceHeap().RecycleDescriptor(m_Descriptors.at(Name));
				m_Descriptors.erase(Name);
			}
		}

		// TODO - Implement move and copy constructors / operators
		TextureFileCache(const TextureFileCache&) = delete;
		TextureFileCache(TextureFileCache&&) = delete;
		TextureFileCache operator=(const TextureFileCache&) = delete;
		TextureFileCache& operator=(TextureFileCache&&) = delete;
	};
}