#pragma once
#include "RenderAssetBase.hpp"
#include "assets/TextureData.hpp"
#include "render_api/D3D12Include.hpp"

namespace aZero::Asset
{
	struct TextureRenderRef
	{
		uint32_t DescriptorIndex = std::numeric_limits<uint32_t>::max();
		bool IsValid() const {
			return DescriptorIndex != std::numeric_limits<uint32_t>::max();
		}
	};

	class Texture : public RenderAssetBase<TextureRenderRef, aZero::Asset::TextureData>
	{
	public:
		Texture() = default;
		Texture(aZero::Asset::TextureData&& data)
			:RenderAssetBase(std::move(data)) {
		}
		Texture(const aZero::Asset::TextureData& data)
			:RenderAssetBase(data) {
		}

		void ClearCachedData() { m_CachedData.Data.clear(); }

	private:

	};
}