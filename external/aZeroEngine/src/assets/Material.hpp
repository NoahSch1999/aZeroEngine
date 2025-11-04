/**
 * @file Material.h
 * @brief Defines the Material asset class.
 *
 * @date 2025-11-04
 * @version 1.0
 */

#pragma once
#include "Texture.hpp"

namespace aZero
{
	namespace Asset
	{
		/**
		 * @class Material
		 * @brief Represents a material asset that defines a mesh's appearance.
		 *
		 * The `Material` class extends `AssetBase` and contains references to texture assets.
		 * It can be loaded from an external material file and provides access to its associated texture handles.
		 *
		 * Instances of `Material` are managed by the AssetManager class.
		 */
		class Material : public AssetBase
		{
		private:
			friend class AssetAllocator<Material>;

			/// @brief Default constructor (used internally by AssetAllocator).
			Material() = default;

			/**
			 * @brief Constructs a new Material asset with the given ID and name (used internally by AssetAllocator).
			 *
			 * @param assetID Unique identifier for the material.
			 * @param name Human-readable material name.
			 */
			Material(AssetID assetID, const std::string& name)
				:AssetBase(assetID, name){}
		public:

			struct Data
			{
				std::string AlbedoTextureName;
				std::string NormalMapName;
				AssetHandle<Texture> AlbedoTexture;
				AssetHandle<Texture> NormalMap;
			};

			bool Load(const std::string& filePath);

			Data m_Data;
		};
	}
}