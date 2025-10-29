#include "Texture.hpp"

#pragma once

namespace aZero
{
	namespace AssetNew
	{
		class Material : public AssetBase
		{
		private:
			friend class AssetAllocator<Material>;
			Material() = default;
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