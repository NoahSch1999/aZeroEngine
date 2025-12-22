#pragma once
#include "graphics_api/resource_type/GPUResourceView.hpp"

namespace aZero
{
	class Engine;
	namespace Rendering
	{
		class RenderContext;

		class RenderSurface : public NonCopyable
		{
			friend Engine;
			friend RenderContext;

		public:
			enum class Type { Color_Target, Depth_Target };
		private:
			Type m_Type;
			std::unique_ptr<D3D12::GPUTexture> m_Texture;
			std::variant<D3D12::RenderTargetView, D3D12::DepthStencilView> m_View;
			bool m_ShouldClear = true;

			// todo Rework so any type can be used etc. maybe ditch a dedicated RenderSurface class entirely and instead let the user handle the resource and rtvs themselves
			RenderSurface(ID3D12Device* Device,
				D3D12::ResourceRecycler* ResourceRecycler,
				D3D12::Descriptor&& Descriptor,
				const DXM::Vector2& Dimensions,
				Type InType,
				std::optional<DXM::Vector4> ClearColor = std::optional<DXM::Vector4>{} // For Type::Color_Target
				)
			{
				DXGI_FORMAT Format;
				D3D12_RESOURCE_FLAGS Flags;
				D3D12_CLEAR_VALUE ClearValue;
				if (InType == Type::Color_Target)
				{
					Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
					Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
					ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

					if (ClearColor.has_value())
					{
						ClearValue.Color[0] = ClearColor.value().x;
						ClearValue.Color[1] = ClearColor.value().y;
						ClearValue.Color[2] = ClearColor.value().z;
						ClearValue.Color[3] = ClearColor.value().w;
						m_ShouldClear = true;
					}
					else
					{
						m_ShouldClear = false;
					}
				}
				else
				{
					Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
					Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
					ClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					if (ClearColor.has_value())
					{
						ClearValue.DepthStencil.Depth = ClearColor->x;
						ClearValue.DepthStencil.Stencil = ClearColor->y;
						m_ShouldClear = true;
					}
					else
					{
						m_ShouldClear = false;
					}
				}


				m_Texture = std::make_unique<D3D12::GPUTexture>(
					Device,
					DXM::Vector3(Dimensions.x, Dimensions.y, 1),
					Format,
					Flags,
					*ResourceRecycler,
					1,
					D3D12_RESOURCE_STATE_COMMON,
					ClearValue
				);

				if (InType == Type::Color_Target)
				{
					m_View.emplace<D3D12::RenderTargetView>(
						D3D12::RenderTargetView(
							Device,
							std::forward<D3D12::Descriptor>(Descriptor),
							*m_Texture.get(), 
							Format, 
							ClearValue
						));
				}
				else
				{
					m_View.emplace<D3D12::DepthStencilView>(
						D3D12::DepthStencilView(
							Device,
							std::forward<D3D12::Descriptor>(Descriptor),
							*m_Texture.get(), 
							Format, 
							ClearValue
						));
				}

				m_Type = InType;
			}

		public:
			RenderSurface() = default;
			RenderSurface(RenderSurface&& Other) noexcept
			{
				m_Texture = std::move(Other.m_Texture);
				m_View = std::move(Other.m_View);
			}

			RenderSurface& operator=(RenderSurface&& Other) noexcept
			{
				if (this != &Other)
				{
					m_Texture = std::move(Other.m_Texture);
					m_View = std::move(Other.m_View);
				}

				return *this;
			}

			Type GetType() const { return m_Type; }

			bool ShouldClear() const { return m_ShouldClear; }

			template<typename DescriptorType>
			const DescriptorType& GetView() const 
			{
				if (!m_Texture.get())
				{
					throw std::runtime_error("RenderSurface is uninitialized");
				}
				return std::get<DescriptorType>(m_View);
			}

			const D3D12::GPUTexture& GetTexture() const 
			{ 
				if (!m_Texture.get())
				{
					throw std::runtime_error("RenderSurface is uninitialized");
				}
				return *m_Texture.get(); 
			}

			void RecordClear(ID3D12GraphicsCommandList* cmdList)
			{
				if (m_ShouldClear)
				{
					if (m_Type == Type::Color_Target)
					{
						cmdList->ClearRenderTargetView(std::get<D3D12::RenderTargetView>(m_View).GetDescriptorHandle(), m_Texture->GetClearValue()->Color, 0, nullptr);
					}
					else if (m_Type == Type::Depth_Target)
					{
						cmdList->ClearDepthStencilView(std::get<D3D12::DepthStencilView>(m_View).GetDescriptorHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, m_Texture->GetClearValue()->DepthStencil.Depth, m_Texture->GetClearValue()->DepthStencil.Stencil, 0, nullptr);
					}
				}
			}
		};
	}
}