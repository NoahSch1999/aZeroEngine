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
		using RenderID = uint32_t;
		constexpr RenderID InvalidRenderID = std::numeric_limits<RenderID>::max();

		class AssetBase
		{
			friend Rendering::Renderer;
		public:
			AssetBase(const std::string& name)
				:m_Name(name) { }

			RenderID GetRenderID() const { return m_RenderID; }

			std::string_view GetName() const { return m_Name; }

		private:
			RenderID m_RenderID = InvalidRenderID;

			// TODO: Remove name?
			std::string m_Name;
		};
	}
}