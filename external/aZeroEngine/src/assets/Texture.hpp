#pragma once
#include "RenderAssetBase.hpp"
#include "render_api/resource/texture/TextureData.hpp"
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

	class Texture : public RenderAssetBase<TextureRenderRef, aZero::RenderAPI::TextureData>
	{
	public:
		Texture() = default;
		Texture(aZero::RenderAPI::TextureData&& data)
			:RenderAssetBase(std::move(data)) {
		}
		Texture(const aZero::RenderAPI::TextureData& data)
			:RenderAssetBase(data) {
		}

	private:

	};
}