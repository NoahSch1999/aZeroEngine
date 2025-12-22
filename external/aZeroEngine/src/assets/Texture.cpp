#include "Texture.hpp"
#include "misc/EngineDebugMacros.hpp"
#include "misc/stb_image.h"

bool aZero::Asset::Texture::Load(const std::string& filePath, DXGI_FORMAT format)
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
		m_Data.TexelData.resize(width * height * 4);
		memcpy(m_Data.TexelData.data(), loadedImage, m_Data.TexelData.size());
		m_Data.Width = width;
		m_Data.Height = height;
		m_Data.NumChannels = channels;
	}
	else
	{
		DEBUG_PRINT("Loading a texture with less than 3 channels isn't supported");
		return false;
	}

	stbi_image_free(loadedImage);
	m_Data.Format = format;

	return true;
}

bool aZero::Asset::NewTexture::Load(const std::string& filePath, DXGI_FORMAT format)
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
		m_Data.TexelData.resize(width * height * 4);
		memcpy(m_Data.TexelData.data(), loadedImage, m_Data.TexelData.size());
		m_Data.Width = width;
		m_Data.Height = height;
		m_Data.NumChannels = channels;
	}
	else
	{
		DEBUG_PRINT("Loading a texture with less than 3 channels isn't supported");
		return false;
	}

	stbi_image_free(loadedImage);
	m_Data.Format = format;

	return true;
}