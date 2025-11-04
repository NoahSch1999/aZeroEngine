/**
 * @file Texture.h
 * @brief Defines the Texture asset class.
 *
 * @date 2025-11-04
 * @version 1.0
 */

#pragma once
#include <vector>
#include "Asset.hpp"


namespace aZero
{
	namespace Asset
	{
		/**
		 * @class Texture
		 * @brief Represents a texture asset containing raw image data.
		 *
		 * The `Texture` class derives from `AssetBase` and stores pixel data along with
		 * image metadata such as width, height, and channel count. Textures can be loaded
		 * from image files using the `Load()` function.
		 * 
		 * Instances are managed by the AssetManager class.
		 */
		class Texture : public AssetBase
		{
		private:
			friend class AssetAllocator<Texture>;
			Texture() = default;
			Texture(AssetID assetID, const std::string& name)
				: AssetBase(assetID, name)
			{

			}
		public:
			struct Data
			{
				std::vector<uint8_t> TexelData;
				uint32_t Width, Height, NumChannels;
			};

			bool Load(const std::string& filePath);

			void RemoveTexelData()
			{
				m_Data.TexelData.clear();
			}

			Data m_Data;
		};
	}
}