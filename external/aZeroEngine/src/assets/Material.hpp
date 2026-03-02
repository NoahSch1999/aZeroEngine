#pragma once
#include "Texture.hpp"

namespace aZero
{
	namespace Rendering
	{
		class Renderer;
	}

	namespace Asset
	{
		class Material : public AssetBase
		{
			friend class Rendering::Renderer;
		public:
			struct Data
			{
				Texture* AlbedoTexture = nullptr;
				Texture* NormalMap = nullptr;
			};

			Material() = default;
			Material(const std::string& name)
				:AssetBase(name) { }

			bool Load(const std::string& filePath);

			void SetAlbedoTexture(Texture* texture) { m_Data.AlbedoTexture = texture; }
			Texture* GetAlbedoTexture() const { return m_Data.AlbedoTexture; }

			void SetNormalMap(Texture* texture) { m_Data.NormalMap = texture; }
			Texture* GetNormalMap() const { return m_Data.NormalMap; }

		private:

			Data m_Data;
		};
	}
}