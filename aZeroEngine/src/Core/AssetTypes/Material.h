#pragma once
#include "Core/D3D12Include.h"
#include "Core/DataStructures/FreelistAllocator.h"
#include "Texture.h"

namespace aZero
{
	namespace Asset
	{
		struct MaterialData
		{
			DXM::Vector3 m_Color = DXM::Vector3(1.f, 1.f, 1.f);
			std::shared_ptr<Texture> m_AlbedoTexture;
			std::shared_ptr<Texture> m_NormalMap;
		};

		struct MaterialRenderData
		{
			DXM::Vector3 m_Color = DXM::Vector3(1.f, 1.f, 1.f);
			uint32_t m_AlbedoDescriptorIndex = 0;
			uint32_t m_NormalMapDescriptorIndex = 0;
		};

		struct MaterialGPUHandle
		{
			DS::FreelistAllocator::AllocationHandle m_MaterialAllocHandle;
		};

		class Material : public RenderFileAsset<MaterialData, MaterialGPUHandle>
		{
		private:

		public:
			Material() = default;

			Material(MaterialData& Material)
			{
				*m_AssetData.get() = Material;
			}

			void SetColor(const DXM::Vector3& Color)
			{
				m_AssetData->m_Color = Color;
			}

			void SetAlbedoTexture(std::shared_ptr<Texture> AlbedoTexture)
			{
				m_AssetData->m_AlbedoTexture = AlbedoTexture;
			}

			void SetNormalMap(std::shared_ptr<Texture> NormalMap)
			{
				m_AssetData->m_NormalMap = NormalMap;
			}

			MaterialRenderData GetRenderData() const 
			{
				MaterialRenderData RenderData;
				RenderData.m_Color = m_AssetData->m_Color;

				if (m_AssetData->m_AlbedoTexture)
				{
					RenderData.m_AlbedoDescriptorIndex = m_AssetData->m_AlbedoTexture->GetDescriptorIndex();
				}
				else
				{
					RenderData.m_AlbedoDescriptorIndex = 0;
				}

				if (m_AssetData->m_NormalMap)
				{
					RenderData.m_NormalMapDescriptorIndex = m_AssetData->m_NormalMap->GetDescriptorIndex();
				}
				else
				{
					RenderData.m_NormalMapDescriptorIndex = 0;
				}

				return RenderData;
			}

			virtual bool HasRenderState() const override
			{
				if (m_AssetGPUHandle->m_MaterialAllocHandle.IsValid())
				{
					return true;
				}

				return false;
			}

			// TODO: Impl
			virtual bool LoadFromFile(const std::string& Path) override
			{
				if (true)
				{
					if (true)
					{
						//this->SetAssetData(std::move(NewTexture));
					}
				}
				return true;
			}
		};
	}
}