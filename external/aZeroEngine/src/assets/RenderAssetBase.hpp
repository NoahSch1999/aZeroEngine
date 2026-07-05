#pragma once

namespace aZero::Rendering { class Renderer; };

namespace aZero::Asset
{
	template<typename RenderRef, typename Data>
	class RenderAssetBase
	{
		friend class aZero::Rendering::Renderer;
	public:
		RenderAssetBase() = default;
		RenderAssetBase(Data&& data)
			:m_CachedData(std::move(data)) {}
		RenderAssetBase(const Data& data)
			:m_CachedData(data) {}

		const RenderRef& GetRenderRef() const { return m_RenderRef; }
		const Data& GetCachedData() const { return m_CachedData; }
		void ClearCachedData() { m_CachedData = Data(); }
	private:
		RenderRef m_RenderRef;
		Data m_CachedData;
	};
}