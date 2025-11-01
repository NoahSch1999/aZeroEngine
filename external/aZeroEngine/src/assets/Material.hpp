#pragma once
#include "Texture.hpp"

namespace aZero
{
	namespace Asset
	{
		class Material : public AssetBase
		{
		private:
			friend class AssetAllocator<Material>;
			Material() = default;
			Material(AssetID assetID, const std::string& name)
				:AssetBase(assetID, name)
			{

			}
		public:
			struct Data
			{
				std::string AlbedoTextureName;
				std::string NormalMapName;
				std::weak_ptr<Texture> AlbedoTexture;
				std::weak_ptr<Texture> NormalMap;
			};

			bool Load(const std::string& filePath);

			Data m_Data;
		};
	}
}