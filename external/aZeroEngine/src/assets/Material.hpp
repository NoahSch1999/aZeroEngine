#pragma once
#include "MaterialData.hpp"

namespace aZero::Asset
{
	struct PBRMaterialData
	{
		uint32_t AlbedoTexture = std::numeric_limits<uint32_t>::max();
		uint32_t NormalMap = std::numeric_limits<uint32_t>::max();
		uint32_t RoughnessMetallic = std::numeric_limits<uint32_t>::max();
		uint32_t GlowMap = std::numeric_limits<uint32_t>::max();
		uint32_t TransparencyMap = std::numeric_limits<uint32_t>::max();
	};

	struct MaterialRenderRef
	{
		uint32_t MaterialIndex = std::numeric_limits<uint32_t>::max();

		bool IsValid() const
		{
			return MaterialIndex != std::numeric_limits<uint32_t>::max();
		}
	};

	class Material : public RenderAssetBase<MaterialRenderRef, Asset::MaterialData>
	{
		friend Rendering::Renderer;
	public:
		Material() = default;
		Material(const Asset::MaterialData& data)
			:RenderAssetBase(data) {
		}
		Material(Asset::MaterialData&& data)
			:RenderAssetBase(std::move(data)) {
		}

		void SetAlbedo(Asset::Texture& texture) { m_CachedData.Info.AlbedoTexture = &texture; }
		void SetNormalMap(Asset::Texture& texture) { m_CachedData.Info.NormalTexture = &texture; }
		void SetMetallicRoughnessTexture(Asset::Texture& texture) { m_CachedData.Info.MetallicRoughnessTexture = &texture; }

		Asset::Texture* GetAlbedoPtr() const { return m_CachedData.Info.AlbedoTexture; }
		Asset::Texture* GetNormalMapPtr() const { return m_CachedData.Info.NormalTexture; }
		Asset::Texture* GetMetallicRoughnessTexturePtr() const { return m_CachedData.Info.MetallicRoughnessTexture; }

		Asset::PBRMaterialData GetFormat_PBR_GPU() const {
			return {
				.AlbedoTexture = this->GetAlbedoPtr() ? this->GetAlbedoPtr()->GetRenderRef().DescriptorIndex : 0xffffffff,
				.NormalMap = this->GetNormalMapPtr() ? this->GetNormalMapPtr()->GetRenderRef().DescriptorIndex : 0xffffffff,
				.RoughnessMetallic = this->GetMetallicRoughnessTexturePtr() ? this->GetMetallicRoughnessTexturePtr()->GetRenderRef().DescriptorIndex : 0xffffffff,
				.GlowMap = 0xffffffff,
				.TransparencyMap = 0xffffffff
			};
		}
	};
}