#pragma once
#include "Texture2D.hpp"
#include "graphics_api/descriptor/ResourceView.hpp"

namespace aZero
{
	namespace Rendering
	{
		class DepthStencilTarget
		{
		public:
			struct Desc
			{

				bool ClearDepthOnDraw = true;
				bool ClearStencilOnDraw = true;
				uint32_t Width, Height;
				float DepthClearValue = 1.f;
				uint8_t StencilClearValue = 0;

				Desc() = default;
				Desc(uint32_t width, uint32_t height, float depthClearValue, uint8_t stencilClearValue, bool clearDepthOnDraw, bool clearStencilOnDraw)
					:Width(width), Height(height), DepthClearValue(depthClearValue), StencilClearValue(stencilClearValue), ClearDepthOnDraw(clearDepthOnDraw), ClearStencilOnDraw(clearStencilOnDraw)
				{

				}

				D3D12_CLEAR_VALUE CreateClearFrom() const
				{
					D3D12_CLEAR_VALUE value = {};
					value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					value.DepthStencil.Depth = DepthClearValue;
					value.DepthStencil.Stencil = StencilClearValue;
					return value;
				}
			};

			DepthStencilTarget() = default;

			DepthStencilTarget(const Desc& desc, ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap, std::optional<RenderAPI::ResourceRecycler*> opt_diResourceRecycler = std::optional<RenderAPI::ResourceRecycler*>{})
				:m_Desc(desc), m_Texture(device, RenderAPI::Texture2D::Desc(desc.Width, desc.Height, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE), opt_diResourceRecycler, desc.CreateClearFrom()), m_View(device, heap, m_Texture, DXGI_FORMAT_D24_UNORM_S8_UINT)
			{

			}

			uint32_t GetHeapIndex() const { return m_View.GetHeapIndex(); }
			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_View.GetCpuHandle(); }
			D3D12_CLEAR_VALUE GetClearValue() const { return m_Desc.CreateClearFrom(); }
			bool ShouldClearDepthOnDraw() const { return m_Desc.ClearDepthOnDraw; }
			bool ShouldClearStencilOnDraw() const { return m_Desc.ClearStencilOnDraw; }

			RenderAPI::Texture2D& GetTexture() { return m_Texture; }
			RenderAPI::Descriptor& GetDescriptor() { return m_View.GetDescriptor(); }
			RenderAPI::DepthStencilTargetView& GetView() { return m_View; }

		private:
			Desc m_Desc;
			RenderAPI::Texture2D m_Texture;
			RenderAPI::DepthStencilTargetView m_View;
		};
	}
}