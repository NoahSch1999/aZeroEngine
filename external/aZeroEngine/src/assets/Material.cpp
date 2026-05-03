#include "Material.hpp"
#include "misc/HelperFunctions.hpp"
#include <fstream>
#include "simdjson.h"

bool aZero::Asset::Material::Save(const std::string& filePath)
{
    const std::string suffix = Helper::GetPathSuffix(filePath);
    if (suffix == "json")
    {
        std::fstream outStream(filePath, std::ios::out | std::ios::trunc);
        if (!outStream.is_open()) {
            return false;
        }

        simdjson::builder::string_builder builder;
        builder.start_object();
        builder.append_raw("\n");

        builder.append_key_value("Albedo", m_Data.AlbedoTexture != nullptr ? m_Data.AlbedoTexture->GetLoadedFilename() : "");
        DEBUG_FUNC(
            [this] {
                if (this->m_Data.AlbedoTexture->GetLoadedFilename().empty())
                {
                    std::cout << "Albedo texture didnt have a loaded path, writing empty to value.\n";
                }
            }
        );

        builder.append_comma();
        builder.append_raw("\n");

        builder.append_key_value("Normal", m_Data.NormalMap != nullptr ? m_Data.NormalMap->GetLoadedFilename() : "");
        DEBUG_FUNC(
            [this] {
                if (this->m_Data.NormalMap->GetLoadedFilename().empty())
                {
                    std::cout << "NormalMap didnt have a loaded path, writing empty to value.\n";
                }
            }
        );

        builder.append_raw("\n");

        builder.end_object();

        outStream << std::string_view(builder);
        outStream.close();
    }

    return true;
}

bool aZero::Asset::Material::LoadFromFile(const std::string& filePath)
{
    const std::string suffix = Helper::GetPathSuffix(filePath);
    if (suffix == "json")
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json = simdjson::padded_string::load(filePath);
        simdjson::ondemand::document document = parser.iterate(json);

        auto albedoJsonString = document["Albedo"].get_string();
        if (albedoJsonString.has_value()) {
            m_LoadedData.AlbedoTexture = albedoJsonString.value();
        }

        auto normalJsonString = document["Normal"].get_string();
        if (normalJsonString.has_value()) {
            m_LoadedData.NormalMap = normalJsonString.value();
        }

        AssetBase::Load(filePath);
    }
	return true;
}