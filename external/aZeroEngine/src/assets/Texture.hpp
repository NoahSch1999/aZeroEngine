#include <vector>
#include "Asset.hpp"

#pragma once

namespace aZero
{
	namespace AssetNew
	{
		class Texture : public AssetBase
		{
		private:
			friend class AssetAllocator<Texture>;
			Texture() = default;
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