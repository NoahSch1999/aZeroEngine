#pragma once
#include "RenderAssetBase.hpp"
#include "misc/stb_image.h"

namespace aZero::Asset
{
	struct TextureData
	{
		std::string FilePath;
		std::vector<uint8_t> TexelData;
		uint32_t Width, Height, NumChannels;
		DXGI_FORMAT Format;

		TextureData() = default;
		TextureData(const std::string& filePath, DXGI_FORMAT format) { this->Load(filePath, format); }

		bool Load(const std::string& filePath, DXGI_FORMAT format)
		{
			std::int32_t width, height, channels;
			stbi_uc* loadedImage = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (!loadedImage)
			{
				DEBUG_PRINT("Failed to load file: " + filePath);
				return false;
			}

			if (channels == 4 || channels == 3)
			{
				TexelData.resize(width * height * 4);
				memcpy(TexelData.data(), loadedImage, TexelData.size());
				Width = width;
				Height = height;
				NumChannels = channels;
			}
			else
			{
				DEBUG_PRINT("Loading a texture with less than 3 channels isn't supported");
				return false;
			}

			stbi_image_free(loadedImage);
			Format = format;

			FilePath = filePath;

			return true;
		}
	};

	struct TextureRenderRef
	{
		uint32_t DescriptorIndex = std::numeric_limits<uint32_t>::max();
		bool IsValid() const {
			return DescriptorIndex != std::numeric_limits<uint32_t>::max();
		}
	};

	class Texture : public RenderAssetBase<TextureRenderRef, TextureData>
	{
	public:
		Texture() = default;
		Texture(TextureData&& data)
			:RenderAssetBase(std::move(data)) {}
		Texture(const Asset::TextureData& data)
			:RenderAssetBase(data) {}

	private:

	};
}