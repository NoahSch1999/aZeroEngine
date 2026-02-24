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
			friend class NewAssetAllocator<Texture>;
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
				DXGI_FORMAT Format;
			};

			bool Load(const std::string& filePath, DXGI_FORMAT format);

			void RemoveTexelData()
			{
				m_Data.TexelData.clear();
			}

			Data m_Data;
		};
	}
}