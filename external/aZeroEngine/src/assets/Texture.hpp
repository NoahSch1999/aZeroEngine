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
		// TODO: Support mips
		struct TextureData
		{
			std::vector<uint8_t> TexelData;
			uint32_t Width, Height, NumChannels;
			DXGI_FORMAT Format;
		};

		class Texture : public AssetBase
		{
			friend class Rendering::Renderer;
		public:

			Texture() = default;
			Texture(AssetID id)
				: AssetBase(id)
			{

			}

			bool Load(const std::string& filePath, DXGI_FORMAT format);

			const TextureData& GetData() {
				return m_Data;
			}

		private:
			TextureData m_Data;
		};
	}
}