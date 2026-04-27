/*
goal:
	- easy per-camera binding
	- easy copy usage

*/
#pragma once
#include <array>
#include "Texture2D.hpp"
#include "graphics_api/descriptor/ResourceView.hpp"

namespace aZero
{
	namespace Rendering
	{
		class RenderTarget
		{
		public:
			struct Desc
			{
				bool ClearOnDraw = true;
				DXGI_FORMAT Format;
				uint32_t Width, Height;
				std::array<float, 4> ClearColor = { 0,0,0,0 };

				Desc() = default;
				Desc(DXGI_FORMAT format, uint32_t width, uint32_t height, std::array<float, 4> clearColor, bool clearOnDraw)
					:Format(format), Width(width), Height(height), ClearColor(clearColor), ClearOnDraw(clearOnDraw)
				{

				}

				D3D12_CLEAR_VALUE CreateClearFrom() const
				{
					D3D12_CLEAR_VALUE value = {};
					value.Format = Format;
					value.Color[0] = ClearColor[0];
					value.Color[1] = ClearColor[1];
					value.Color[2] = ClearColor[2];
					value.Color[3] = ClearColor[3];
					return value;
				}
			};

			RenderTarget() = default;

			RenderTarget(const Desc& desc, ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap, std::optional<RenderAPI::ResourceRecycler*> opt_diResourceRecycler = std::optional<RenderAPI::ResourceRecycler*>{})
				:m_Desc(desc), m_Texture(device, RenderAPI::Texture2D::Desc(desc.Width, desc.Height, desc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET), opt_diResourceRecycler, desc.CreateClearFrom()), m_View(device, heap, m_Texture, desc.Format)
			{ }

			uint32_t GetHeapIndex() const { return m_View.GetHeapIndex(); }
			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_View.GetCpuHandle(); }
			D3D12_CLEAR_VALUE GetClearValue() const { return m_Desc.CreateClearFrom(); }
			bool ShouldClearOnDraw() const { return m_Desc.ClearOnDraw; }

			RenderAPI::Texture2D& GetTexture() { return m_Texture; }
			RenderAPI::Descriptor& GetDescriptor() { return m_View.GetDescriptor(); }
			RenderAPI::RenderTargetView& GetView() { return m_View; }

		private:
			Desc m_Desc;
			RenderAPI::Texture2D m_Texture;
			RenderAPI::RenderTargetView m_View;
		};
	}
}