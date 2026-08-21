#pragma once
#include <filesystem>
#include <ktx.h>
#include "misc/stb_image.h"
#include "render_api/Enums.hpp"

namespace aZero::Asset
{
	struct TextureData
	{
		enum class TextureType {
			Texture1D,
			Texture1DArray,
			Texture2D,
			Texture2DArray,
			TextureCube,
			TextureCubeArray,
			Texture3D
		};

		struct MipLevel
		{
			uint32_t Offset = 0u, RowPitch = 0u, SlicePitch = 0u;
		};

		TextureType Type;
		std::filesystem::path FilePath;
		aZero::RenderAPI::TEXTURE_FORMAT Format = aZero::RenderAPI::TEXTURE_FORMAT::UNKNOWN;
		uint32_t Width = 0u, Height = 0u, DepthOrArrayCount = 0u, Faces = 0u;
		std::vector<MipLevel> MipPitchData;
		std::vector<uint8_t> Data;
		std::string Name;

		TextureData() = default;

		TextureData(const std::filesystem::path& path) { this->CreateFromFile(path); }

		bool CreateFromFile(const std::filesystem::path& path)
		{
			if (path.extension() == ".ktx2")
			{
				ktxTexture2* ktxTex = nullptr;
				KTX_error_code result =
					ktxTexture2_CreateFromNamedFile(
						path.generic_string().c_str(),
						KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
						&ktxTex);

				if (result != KTX_SUCCESS)
				{
					return false;
				}

				if (ktxTexture2_NeedsTranscoding(ktxTex)) {

					const bool isSRGB = ktxTexture2_GetTransferFunction_e(ktxTex) == khr_df_transfer_e::KHR_DF_TRANSFER_SRGB;

					result = ktxTexture2_TranscodeBasis( // todo Maybe we need to know if its srgb or not...?
						ktxTex,
						KTX_TTF_BC7_RGBA,
						KTX_TF_HIGH_QUALITY);

					if (result != KTX_SUCCESS)
					{
						ktxTexture2_Destroy(ktxTex);
						return false;
					}

					Format = isSRGB ? aZero::RenderAPI::TEXTURE_FORMAT::BC7_UNORM_SRGB : aZero::RenderAPI::TEXTURE_FORMAT::BC7_UNORM;
				}
				else {
					Format = aZero::RenderAPI::FromVK_Format(ktxTex->vkFormat);
				}

				if (ktxTex->isCubemap) {
					Type = ktxTex->isArray ? TextureType::TextureCubeArray : TextureType::TextureCube;
				}
				else
				{
					if (ktxTex->baseDepth > 1) {
						Type = TextureType::Texture3D;
					}
					else if (ktxTex->baseHeight > 1) {
						Type = ktxTex->isArray ? TextureType::Texture2DArray : TextureType::Texture2D;
					}
					else {
						Type = ktxTex->isArray ? TextureType::Texture1DArray : TextureType::Texture1D;
					}
				}

				Faces = ktxTex->numFaces;
				Width = ktxTex->baseWidth;
				Height = ktxTex->baseHeight;
				DepthOrArrayCount = ktxTex->isArray // Is array?
					? ktxTex->numLayers // Yes => Array count
						: Type == TextureType::Texture3D ? // No => Is it 3D? Yes => Get depth, No => Set 0 as depth.
					ktxTex->baseDepth : 0.f;

				MipPitchData.resize(ktxTex->numLevels);
				for (uint32_t mip = 0; mip < ktxTex->numLevels; mip++)
				{
					ktx_size_t offset;

					ktxTexture_GetImageOffset(
						reinterpret_cast<ktxTexture*>(ktxTex),
						mip,
						0,
						0,
						&offset);

					const ktx_size_t size = ktxTexture_GetImageSize(
						reinterpret_cast<ktxTexture*>(ktxTex),
						mip);

					// Reduces textures mip0 width and height by a power of 2 per mip level
					uint32_t mipWidth = std::max(1u, ktxTex->baseWidth >> mip);
					uint32_t mipHeight = std::max(1u, ktxTex->baseHeight >> mip);

					MipPitchData[mip].Offset = offset;
					MipPitchData[mip].SlicePitch = size;

					if (Format == aZero::RenderAPI::TEXTURE_FORMAT::BC7_UNORM ||
						Format == aZero::RenderAPI::TEXTURE_FORMAT::BC7_UNORM_SRGB)
					{
						// BC7 - Stores the texture in blocks of 16 bytes.
						// We calculate the number of blocks and multiply by 16 to get the width.
						// We add 3 so we always will round up. Ex.
						/*
							width = 10
							10 / 4 = 2.5
							integers are truncated so we will get 8 (4*2) which isnt the correct rowpitch (it should be 10)
							we add 3 to always get a value that when rounded down becomes atleast the same as width
							Ex:
							width = 10
							(10 + 3) / 4
							13 / 4 = 3.25 => 3
							4 * 3 = 12 and now we have the same or more than 10.
							
						*/
						MipPitchData[mip].RowPitch =
							((mipWidth + 3) / 4) * 16;
					}
					else
					{

						MipPitchData[mip].RowPitch = mipWidth * 4;
					}
				}
				Data.resize(ktxTex->dataSize);
				std::memcpy(Data.data(), ktxTex->pData, ktxTex->dataSize);

				ktxTexture2_Destroy(ktxTex);
			}
			else if (path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg")
			{
				std::int32_t width, height, channels;
				stbi_uc* loadedImage = stbi_load(path.generic_string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
				if (!loadedImage)
				{
					return false;
				}

				Data.resize(width * height * STBI_rgb_alpha);
				memcpy(Data.data(), loadedImage, Data.size());
				Width = width;
				Height = height;

				MipLevel miplevel;
				miplevel.RowPitch = width * sizeof(DWORD);
				miplevel.SlicePitch = miplevel.RowPitch * height;
				MipPitchData.emplace_back(miplevel);

				if (path.filename().generic_string().ends_with(std::string("_BaseColor") + path.extension().generic_string()))
				{
					Format = aZero::RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM_SRGB;
				}
				else
				{
					Format = aZero::RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM;
				}

				stbi_image_free(loadedImage);

			}

			FilePath = path;
			Name = path.extension().generic_string();

			return true;
		}

		bool LoadFromMemory(const std::filesystem::path& path, const std::byte* memoryPtr, std::size_t numBytes, std::size_t offsetIntoMemoryPtr = 0)
		{
			std::int32_t width, height, channels;
			stbi_uc* loadedImage = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(memoryPtr + offsetIntoMemoryPtr),
				static_cast<int32_t>(numBytes), &width, &height, &channels, STBI_rgb_alpha);

			Data.resize(width * height * STBI_rgb_alpha);
			memcpy(Data.data(), loadedImage, Data.size());
			Width = width;
			Height = height;

			MipLevel miplevel;
			miplevel.RowPitch = width * sizeof(DWORD);
			miplevel.SlicePitch = miplevel.RowPitch * height;
			MipPitchData.emplace_back(miplevel);

			if (path.filename().generic_string().ends_with(std::string("_BaseColor") + path.extension().generic_string()))
			{
				Format = aZero::RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM_SRGB;
			}
			else
			{
				Format = aZero::RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM;
			}

			stbi_image_free(loadedImage);

			FilePath = path;
			Name = path.extension().generic_string();

			return true;
		}
	};
}