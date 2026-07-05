#pragma once
#include "Texture.hpp"
#include "simdjson.h"
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

	struct MaterialData
	{
		std::string FilePath;
		std::string AlbedoTexture;
		std::string NormalMap;

		MaterialData() = default;
		MaterialData(const std::string& filePath) { this->Load(filePath); }

		bool Load(const std::string& filePath)
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
		}
	};

	class Material : public RenderAssetBase<MaterialRenderRef, Asset::MaterialData>
	{
		friend Rendering::Renderer;
	public:
		Material() = default;
		Material(const Asset::MaterialData& data)
			:RenderAssetBase(data) {}
		Material(Asset::MaterialData&& data)
			:RenderAssetBase(std::move(data)) {}

		void SetAlbedo(Asset::Texture& albedo) { m_AlbedoRef = &albedo; }
		void SetNormalMap(Asset::Texture& normalMap) { m_NormalRef = &normalMap; }

		Asset::Texture* GetAlbedoPtr() const { return m_AlbedoRef; }
		Asset::Texture* GetNormalMapPtr() const { return m_NormalRef; }

	private:
		Asset::Texture* m_AlbedoRef = nullptr;
		Asset::Texture* m_NormalRef = nullptr;
	};
}