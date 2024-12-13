#include "Texture.h"
#include "ThirdParty/DirectXTK12/Inc/WICTextureLoader.h"
#include "Core/Misc/stb_Image_Include.h"

bool aZero::Asset::LoadTextureFromFile(const std::string& Path, TextureData& OutData)
{
	// TODO: Change when we have a more ideal texture loading way
	int32_t Width, Height, Channels;
	stbi_uc* LoadedImage = stbi_load(Path.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
	if (!LoadedImage)
	{
		DEBUG_PRINT("LoadTextureFromFile() => Failed to load file: " + Path);
		return false;
	}

	if (Channels == 4 || Channels == 3)
	{
		OutData.m_Data.resize(Width * Height * 4);
		memcpy(OutData.m_Data.data(), LoadedImage, OutData.m_Data.size());
		OutData.m_Dimensions.x = Width;
		OutData.m_Dimensions.y = Height;
		OutData.m_Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (/*Channels == 3*/false)
	{
		// TODO: Swizzle
		OutData.m_Data.resize(Width * Height * 4);
		for (int y = 0; y < Height; ++y)
		{
			uint8_t* dest_row = OutData.m_Data.data() + (y * Width * 4);
			uint8_t* src_row = LoadedImage + (y * Width * 3);
			for (int x = 0; x < Width; ++x)
			{
				dest_row[x * 4 + 0] = src_row[x * 3 + 2];
				dest_row[x * 4 + 1] = src_row[x * 3 + 1];
				dest_row[x * 4 + 2] = src_row[x * 3 + 0];
				dest_row[x * 4 + 3] = 255;
			}
		}

		OutData.m_Dimensions.x = Width;
		OutData.m_Dimensions.y = Height;
		OutData.m_Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	}
	else
	{
		DEBUG_PRINT("LoadTextureFromFile() => Loading a texture with less than 3 channels isn't supported");
		return false;
	}

	stbi_image_free(LoadedImage);

	return true;
}
