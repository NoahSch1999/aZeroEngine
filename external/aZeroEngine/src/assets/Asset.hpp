#pragma once
#include "graphics_api/D3D12Include.hpp"

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

		using AssetID = uint32_t;
		using RenderID = uint32_t;

		class AssetBase
		{
			friend Rendering::Renderer;
		private:
			RenderID m_RenderID = std::numeric_limits<RenderID>::max();
			AssetID m_AssetID;
			std::string m_Name;

		public:
			AssetBase(AssetID assetID, const std::string& name)
				:m_AssetID(assetID), m_Name(name) { }

			AssetID GetAssetID() const { return m_AssetID; }
			RenderID GetRenderID() const { return m_RenderID; }
			const std::string& GetName() const { return m_Name; }
		};
	}
}