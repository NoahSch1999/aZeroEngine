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
			std::unique_ptr<D3D12::GPUTexture> m_Texture;
			std::variant<D3D12::RenderTargetView, D3D12::DepthStencilView> m_View;

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
					Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
					ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

					if (ClearColor.has_value())
					{
						ClearValue.Color[0] = ClearColor.value().x;
						ClearValue.Color[1] = ClearColor.value().y;
						ClearValue.Color[2] = ClearColor.value().z;
						ClearValue.Color[3] = ClearColor.value().w;
					}
				}
				else
				{
					Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
					Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
					ClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					ClearValue.DepthStencil.Depth = 1.f;
					ClearValue.DepthStencil.Stencil = 0;
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
		};
	}
}