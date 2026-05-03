#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"
#include "misc/HelperFunctions.hpp"

namespace aZero
{
	namespace Rendering
	{
		class ResourceManager;
	}

	namespace Asset
	{
		inline std::string GetMeshDirectoryPath() { return PROJECT_DIRECTORY + std::string("assets/meshes/"); }
		inline std::string GetTextureDirectoryPath() { return PROJECT_DIRECTORY + std::string("assets/textures/"); }
		inline std::string GetMaterialDirectoryPath() { return PROJECT_DIRECTORY + std::string("assets/materials/"); }

		using AssetID = uint32_t;
		constexpr AssetID InvalidAssetID = std::numeric_limits<AssetID>::max();

		using RenderID = uint16_t;
		constexpr RenderID InvalidRenderID = std::numeric_limits<RenderID>::max();

		class AssetBase : public NonCopyable
		{
			friend Rendering::ResourceManager;
		public:
			AssetBase() 
				:m_AssetID(m_IncrementingID.fetch_add(1, std::memory_order_relaxed)) { }

			AssetID GetAssetID() const { return m_AssetID; }
			RenderID GetRenderID() const { return m_RenderID; }
			const std::string& GetLoadedFilename() const { return m_LoadedFilename; }
		protected:
			void Load(const std::string& filePath)
			{
				m_LoadedFilename = Helper::GetFilenameFromPath(filePath);
			}
		private:
			static inline std::atomic<AssetID> m_IncrementingID = 0;
			AssetID m_AssetID = InvalidAssetID;
			RenderID m_RenderID = InvalidRenderID;
			std::string m_LoadedFilename;
		};
	}
}