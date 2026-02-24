#pragma once
#include "Texture.hpp"

namespace aZero
{
	namespace Asset
	{
		class Material : public AssetBase
		{
		private:
			friend class NewAssetAllocator<Material>;

			Material() = default;
			Material(AssetID assetID, const std::string& name)
				:AssetBase(assetID, name){}

		public:

			struct Data
			{
				std::string AlbedoTextureName;
				std::string NormalMapName;
				Texture* AlbedoTexture = nullptr;
				Texture* NormalMap = nullptr;
			};

			bool Load(const std::string& filePath);

			Data m_Data;
		};
	}
}