#pragma once
#include "NEW_RenderAssetBase.hpp"
#include "renderer/Renderer.hpp"

namespace aZero
{
	namespace NEW_Asset
	{
		struct TextureData
		{
			std::string FilePath;
			std::vector<uint8_t> TexelData;
			uint32_t Width, Height, NumChannels;
			DXGI_FORMAT Format;
		};

		struct TextureRenderRef
		{
			uint32_t DescriptorIndex = std::numeric_limits<uint32_t>::max();
			bool IsValid() const {
				return DescriptorIndex != std::numeric_limits<uint32_t>::max();
			}
		};

		class Texture : public RenderAssetBase<TextureRenderRef, TextureData>
		{
		public:
			Texture() = default;
			Texture(TextureData&& data)
				:RenderAssetBase(std::move(data)) {}

		private:

		};
	}

	template<>
	inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<NEW_Asset::Texture>(NEW_Asset::Texture& texture)
	{
		// todo Impl update of existing asset
		if (texture.GetRenderRef().IsValid())
		{
			throw;
		}
		FrameContext& context = this->GetCurrentContext();
		texture.m_RenderRef.DescriptorIndex = m_RenderAssetManager->UpdateRenderState(m_diDevice, context.GetCommandList(), m_ResourceRecycler, m_ResourceHeap,
			texture.GetCachedData().TexelData, texture.GetCachedData().Width, texture.GetCachedData().Height, texture.GetCachedData().Format);
		m_DirectCommandQueue.ExecuteCommandList(context.GetCommandList());
	}

	template<>
	inline void aZero::Rendering::Renderer::UnregisterAsset<NEW_Asset::Texture>(NEW_Asset::Texture& texture)
	{
		m_RenderAssetManager->RemoveTextureAsset(texture.GetRenderRef().DescriptorIndex);

		// todo Impl recycle that doesnt force flush of descriptors
		this->FlushRenderCommands();
	}
}