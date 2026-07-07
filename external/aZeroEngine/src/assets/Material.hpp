#pragma once
#include "Texture.hpp"
#include <simdjson.h>
#include <fstream>

namespace aZero::Asset
{
	struct MaterialRenderRef
	{
		uint32_t MaterialIndex = std::numeric_limits<uint32_t>::max();

		bool IsValid() const
		{
			return MaterialIndex != std::numeric_limits<uint32_t>::max();
		}
	};

	struct MaterialInfo
	{
		Asset::Texture* AlbedoTexture = nullptr;
		Asset::Texture* NormalTexture = nullptr;
		Asset::Texture* MetallicRoughnessTexture = nullptr;
		float RoughnessFactor;
		float MetallicFactor;
	};

	struct MaterialData
	{
		std::string FilePath;
		std::string Name;
		MaterialInfo Info;

		MaterialData() = default;
		//MaterialData(const std::string& filePath) { this->Load(filePath); }

		/*bool Load(const std::string& filePath)
		{
			const std::string suffix = Helper::GetPathSuffix(filePath);
			if (suffix == "json")
			{
				simdjson::ondemand::parser parser;
				auto json = simdjson::padded_string::load(filePath);
				if (json.has_value())
				{
					simdjson::ondemand::document document = parser.iterate(json);

					auto albedoJsonString = document["Albedo"].get_string();
					if (albedoJsonString.has_value()) {
						AlbedoTexture = albedoJsonString.value();
					}

					auto normalJsonString = document["Normal"].get_string();
					if (normalJsonString.has_value()) {
						NormalMap = normalJsonString.value();
					}
					FilePath = filePath;
					return true;
				}

			}
			return false;
		}*/
	};

	class Material : public RenderAssetBase<MaterialRenderRef, Asset::MaterialData>
	{
		friend Rendering::Renderer;
	public:
		Material() = default;
		Material(const Asset::MaterialData& data)
			:RenderAssetBase(data) {
			const auto& cachedData = this->GetCachedData();
			m_Info = cachedData.Info;
			m_Name = cachedData.Name;
			m_FilePath = cachedData.FilePath;
		}
		Material(Asset::MaterialData&& data)
			:RenderAssetBase(std::move(data)) {
			const auto& cachedData = this->GetCachedData();
			m_Info = cachedData.Info;
			m_Name = cachedData.Name;
			m_FilePath = cachedData.FilePath;
		}

		void SetAlbedo(Asset::Texture& texture) { m_Info.AlbedoTexture = &texture; }
		void SetNormalMap(Asset::Texture& texture) { m_Info.NormalTexture = &texture; }
		void SetMetallicRoughnessTexture(Asset::Texture& texture) { m_Info.MetallicRoughnessTexture = &texture; }

		Asset::Texture* GetAlbedoPtr() const { return m_Info.AlbedoTexture; }
		Asset::Texture* GetNormalMapPtr() const { return m_Info.NormalTexture; }
		Asset::Texture* GetMetallicRoughnessTexturePtr() const { return m_Info.MetallicRoughnessTexture; }

	private:
		MaterialInfo m_Info;
	};
}