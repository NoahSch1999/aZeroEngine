/**
 * @file AssetBase.h
 * @brief Defines the base class for all asset types.
 *
 * @date 2025-11-04
 * @version 1.0
 */

#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "AssetHandle.hpp"

namespace aZero
{
	namespace Rendering
	{
		class Renderer;
	}

	namespace Asset
	{
		template<typename T>
		class AssetAllocator;

		/** 
		* @brief Unique identifier for an asset within the asset system.
		* 
		* This will be incremented for each new asset of the same type that is created. This means the max assets created during a single session is max of AssetID.
		*/
		using AssetID = uint32_t;

		/// @brief Unique identifier for the renderer
		using RenderID = uint32_t;

		/**
		 * @class AssetBase
		 * @brief Base class providing common metadata for all assets.
		 *
		 * The `AssetBase` class serves as the foundation for all asset types. It contains core metadata such as the asset’s unique ID,
		 * rendering ID, and name.
		 */
		class AssetBase
		{
			friend Rendering::Renderer;
		private:
			/**
			 * @brief Unique identifier for the renderer.
			 *
			 * Initialized to the maximum value of `RenderID` to indicate that
			 * the asset is not currently bound to the renderer.
			 */
			RenderID m_RenderID = std::numeric_limits<RenderID>::max();

			/// @brief Unique identifier for this asset.
			AssetID m_AssetID;

			/// @brief Human-readable name of the asset.
			std::string m_Name;

		public:
			/**
			 * @brief Constructs a new AssetBase instance.
			 *
			 * @param assetID Unique asset identifier.
			 * @param name Human-readable name of the asset.
			 */
			AssetBase(AssetID assetID, const std::string& name)
				:m_AssetID(assetID), m_Name(name) { }

			/**
			 * @brief Retrieves the unique ID of this asset.
			 * @return The `AssetID` associated with this asset.
			 */
			AssetID GetAssetID() const { return m_AssetID; }

			/**
			 * @brief Retrieves the render ID used by the renderer.
			 * @return The `RenderID` currently associated with this asset.
			 */
			RenderID GetRenderID() const { return m_RenderID; }

			/**
			 * @brief Retrieves the name of this asset.
			 * @return A string_view to the asset’s name.
			 */
			std::string_view GetName() const { return m_Name; }
		};
	}
}