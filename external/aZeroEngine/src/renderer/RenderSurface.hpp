#pragma once
#include <variant>

#include "graphics_api/resource/texture/Texture2D.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	class Engine;
	namespace Rendering
	{
		template<typename TargetDesc>
		class OutputTargetBase
		{
		public:
			OutputTargetBase() = default;

			// TODO: Make it so the desc can be used to add more flags for the texture
			OutputTargetBase(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap, RenderAPI::ResourceRecycler& recycler, const TargetDesc& desc, bool shouldClear, D3D12_RESOURCE_FLAGS flags)
				:m_Texture(RenderAPI::Texture2D(device, RenderAPI::Texture2D::Desc(static_cast<uint32_t>(desc.dimensions.x), static_cast<uint32_t>(desc.dimensions.y), desc.format, flags), &recycler, desc.GetDX())),
				m_Descriptor(heap.CreateDescriptor()), m_Desc(desc), m_ClearState(shouldClear) { }

			bool GetClearState() const { return m_ClearState; }
			void SetClearState(bool shouldClear) {
				m_ClearState = shouldClear;
			}

			TargetDesc GetDesc() const { return m_Desc; }

			const RenderAPI::Texture2D& GetTexture() const { return m_Texture; }
			RenderAPI::Descriptor& GetTargetDescriptor() { return m_Descriptor; }

		protected:
			RenderAPI::Descriptor m_Descriptor;
			RenderAPI::Texture2D m_Texture;
			TargetDesc m_Desc;

		private:
			bool m_ClearState;
		};

		struct DepthTargetDesc {
			DXM::Vector2 dimensions;
			float depthClearValue;
			uint8_t stencilClearValue;
			DXGI_FORMAT format;

			DepthTargetDesc() = default;
			DepthTargetDesc(const DXM::Vector2& dimensions, float depthClearValue, uint8_t stencilClearValue, DXGI_FORMAT format)
				:dimensions(dimensions), depthClearValue(depthClearValue), stencilClearValue(stencilClearValue), format(format) { }

			D3D12_CLEAR_VALUE GetDX() const
			{
				D3D12_CLEAR_VALUE clearValue;
				clearValue.DepthStencil.Depth = depthClearValue;
				clearValue.DepthStencil.Stencil = stencilClearValue;
				clearValue.Format = format;
				return clearValue;
			}
		};

		class DepthTarget : public OutputTargetBase<DepthTargetDesc>
		{
		public:
			using Desc = DepthTargetDesc;

			DepthTarget() = default;
			DepthTarget(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap, RenderAPI::ResourceRecycler& recycler, const DepthTargetDesc& desc, bool shouldClear)
				:OutputTargetBase(device, heap, recycler, desc, shouldClear, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
			{
				this->CreateView(device);
			}

		private:
			void CreateView(ID3D12DeviceX* device)
			{
				D3D12_DEPTH_STENCIL_VIEW_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Format = m_Desc.format;
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = 0;

				device->CreateDepthStencilView(m_Texture.GetResource(), &desc, m_Descriptor.GetCpuHandle());
			}
		};

		struct RenderTargetDesc {
			DXM::Vector4 colorClearValue;
			DXM::Vector2 dimensions;
			DXGI_FORMAT format;

			RenderTargetDesc() = default;
			RenderTargetDesc(const DXM::Vector4& colorClearValue, const DXM::Vector2& dimensions, DXGI_FORMAT format)
				:colorClearValue(colorClearValue), dimensions(dimensions), format(format) { }

			D3D12_CLEAR_VALUE GetDX() const
			{
				D3D12_CLEAR_VALUE clearValue;
				clearValue.Format = format;
				clearValue.Color[0] = colorClearValue.x;
				clearValue.Color[1] = colorClearValue.y;
				clearValue.Color[2] = colorClearValue.z;
				clearValue.Color[3] = colorClearValue.w;

				return clearValue;
			}
		};

		class RenderTarget : public OutputTargetBase<RenderTargetDesc>
		{
		public:
			using Desc = RenderTargetDesc;

			RenderTarget() = default;
			RenderTarget(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap, RenderAPI::ResourceRecycler& recycler, const RenderTargetDesc& desc, bool shouldClear)
				: OutputTargetBase(device, heap, recycler, desc, shouldClear, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
			{
				this->CreateView(device);
			}

		private:
			void CreateView(ID3D12DeviceX* device)
			{
				D3D12_RENDER_TARGET_VIEW_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Format = m_Desc.format;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = 0;
				desc.Texture2D.PlaneSlice = 0;

				device->CreateRenderTargetView(m_Texture.GetResource(), &desc, m_Descriptor.GetCpuHandle());
			}
		};
	}
}