#pragma once
#include <array>
#include "Texture2D.hpp"
#include "render_api/descriptor/ResourceView.hpp"
#include "render_api/Enums.hpp"

namespace aZero
{
	namespace Rendering
	{
		class RenderTarget
		{
		public:
			struct Desc
			{
				uint32_t Width, Height;
				std::array<float, 4> ClearColor = { 0,0,0,0 };
				RenderAPI::TEXTURE_FORMAT Format;

				Desc() = default;
				Desc(uint32_t width, uint32_t height, std::array<float, 4> clearColor, RenderAPI::TEXTURE_FORMAT format)
					:Width(width), Height(height), ClearColor(clearColor), Format(format) { }
			};

			RenderTarget() = default;

			RenderTarget(const Desc& desc, ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap, std::optional<RenderAPI::ResourceRecycler*> opt_diResourceRecycler = std::optional<RenderAPI::ResourceRecycler*>{})
			{
				m_ClearValue.Color[0] = desc.ClearColor[0];
				m_ClearValue.Color[1] = desc.ClearColor[1];
				m_ClearValue.Color[2] = desc.ClearColor[2];
				m_ClearValue.Color[3] = desc.ClearColor[3];
				m_ClearValue.Format = RenderAPI::ToDX_Format(desc.Format);

				m_Texture = RenderAPI::Texture2D(device, RenderAPI::Texture2D::Desc(desc.Width, desc.Height, m_ClearValue.Format,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, D3D12_RESOURCE_STATE_RENDER_TARGET), opt_diResourceRecycler, m_ClearValue);
				m_View = RenderAPI::RenderTargetView(device, heap, m_Texture, m_ClearValue.Format);
			}

			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_View.GetCpuHandle(); }
			RenderAPI::Texture2D& GetTexture() { return m_Texture; }
			RenderAPI::Descriptor& GetDescriptor() { return m_View.GetDescriptor(); }
			D3D12_CLEAR_VALUE GetClearValue() const { return m_ClearValue; }

		private:
			D3D12_CLEAR_VALUE m_ClearValue;
			RenderAPI::Texture2D m_Texture;
			RenderAPI::RenderTargetView m_View;
		};
	}
}