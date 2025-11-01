#pragma once
#include <vector>
#include "Asset.hpp"


namespace aZero
{
	namespace Asset
	{
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