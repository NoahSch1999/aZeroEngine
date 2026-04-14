#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"

namespace aZero
{
	namespace Rendering
	{
		class ResourceManager;
	}

	namespace Asset
	{
		using AssetID = uint32_t;
		constexpr AssetID InvalidAssetID = std::numeric_limits<AssetID>::max();

		using RenderID = uint32_t;
		constexpr RenderID InvalidRenderID = std::numeric_limits<RenderID>::max();

		class AssetBase : public NonCopyable
		{
			friend Rendering::ResourceManager;
		public:
			AssetBase() = default;
			AssetBase(AssetID id)
				:m_AssetID(id) { }

			AssetID GetAssetID() const { return m_AssetID; }
			RenderID GetRenderID() const { return m_RenderID; }

		private:
			AssetID m_AssetID = InvalidAssetID;
			RenderID m_RenderID = InvalidRenderID;
		};
	}
}