#pragma once
#include "Texture.hpp"
#include <simdjson.h>
#include <fstream>

namespace aZero::Asset
{
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
		std::string Name;
		std::string FilePath;
		MaterialInfo Info;

		MaterialData() = default;
	};
}