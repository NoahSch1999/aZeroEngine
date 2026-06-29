#pragma once
#include "renderer/Renderer.hpp"
#include "renderer/NEW_RenderAssetManager.hpp"

namespace aZero
{
	namespace NEW_Asset
	{
		template<typename RenderRef, typename Data>
		class RenderAssetBase
		{
			friend class aZero::Rendering::Renderer;
		public:
			RenderAssetBase() = default;
			RenderAssetBase(Data&& data)
				:m_CachedData(std::move(data)) {}

			const RenderRef& GetRenderRef() const { return m_RenderRef; }
			const Data& GetCachedData() const { return m_CachedData; }
			void ClearCachedData() { m_CachedData = Data(); }
		private:
			RenderRef m_RenderRef;
			Data m_CachedData;
		};
	}
}