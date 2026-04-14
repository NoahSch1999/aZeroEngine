#pragma once
#include <vector>
#include "Asset.hpp"

namespace aZero
{
	namespace Rendering
	{
		class Renderer;
	}

	namespace Asset
	{
		class Texture : public AssetBase
		{
			friend class Rendering::Renderer;
		public:
			struct Data
			{
				std::vector<uint8_t> TexelData;
				uint32_t Width, Height, NumChannels;
				DXGI_FORMAT Format;
			};

			Texture() = default;
			Texture(AssetID id)
				: AssetBase(id)
			{

			}

			bool Load(const std::string& filePath, DXGI_FORMAT format);

			void RemoveTexelData()
			{
				m_Data.TexelData.clear();
			}

		private:
			Data m_Data;
		};
	}
}