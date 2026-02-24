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
		using AssetID = uint32_t;
		using RenderID = uint32_t;
		constexpr RenderID InvalidRenderID = std::numeric_limits<RenderID>::max();

		template<typename T>
		class NewAssetAllocator;

		class AssetBase
		{
			friend Rendering::Renderer;
		public:
			AssetBase(AssetID assetID, const std::string& name)
				:m_AssetID(assetID), m_Name(name) {
			}

			AssetID GetAssetID() const { return m_AssetID; }

			RenderID GetRenderID() const { return m_RenderID; }

			std::string_view GetName() const { return m_Name; }

		private:
			RenderID m_RenderID = InvalidRenderID;

			AssetID m_AssetID;

			std::string m_Name;
		};
	}
}