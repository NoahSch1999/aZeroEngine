#include "graphics_api/D3D12Include.hpp"

#pragma once

namespace aZero
{
	namespace AssetNew
	{
		template<typename T>
		class AssetAllocator;

		using AssetID = uint32_t;
		using RenderID = uint32_t;

		class AssetBase
		{
		public:
			AssetID m_AssetID;
			RenderID m_RenderID = std::numeric_limits<AssetNew::RenderID>::max(); // TODO: Make accessible to the renderer
			std::string m_Name;
		};
	}
}