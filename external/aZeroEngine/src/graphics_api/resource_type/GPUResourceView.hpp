#pragma once
#include "GPUResource.hpp"
#include "graphics_api/DescriptorHeap.hpp"
#include <variant>

namespace aZero
{
	namespace D3D12
	{
		class GPUResourceView : public NonCopyable
		{
		protected:
			D3D12::Descriptor m_Descriptor;
			std::variant<D3D12::GPUBuffer*, D3D12::GPUTexture*> m_Resource;

		public:
			GPUResourceView() = default;

			GPUResourceView(GPUResourceView&& Other) noexcept
			{
				m_Descriptor = std::move(Other.m_Descriptor);
				m_Resource = std::move(Other.m_Resource);
			}

			GPUResourceView& operator=(GPUResourceView&& Other) noexcept
			{
				if (this != &Other)
				{
					m_Descriptor = std::move(Other.m_Descriptor);
					m_Resource = std::move(Other.m_Resource);
				}
				return *this;
			}

			uint32_t GetDescriptorIndex() const { return m_Descriptor.GetHeapIndex(); }
			D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return m_Descriptor.GetCPUHandle(); }

			template<typename ResourceType>
			const ResourceType* GetResource() const
			{
				return std::get<ResourceType*>(m_Resource);
			}
		};

		class UnorderedAccessView : public GPUResourceView
		{
		private:

		public:
			UnorderedAccessView() = default;

			UnorderedAccessView(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
				this->Init(Device, std::forward<D3D12::Descriptor>(Descriptor), Buffer, Format, FirstElement, NumElements, BytesPerElement, TreatAsRawBuffer);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
				m_Resource.emplace<D3D12::GPUBuffer*>(&Buffer);
				m_Descriptor = std::move(Descriptor);

				D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
				ZeroMemory(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));
				Desc.Format = Format;
				Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				Desc.Buffer.FirstElement = FirstElement;
				Desc.Buffer.NumElements = NumElements;
				Desc.Buffer.StructureByteStride = BytesPerElement;
				Desc.Buffer.Flags = TreatAsRawBuffer ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

				Device->CreateUnorderedAccessView(Buffer.GetResource(), nullptr, &Desc, m_Descriptor.GetCPUHandle());
			}

			UnorderedAccessView(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0,
				uint32_t PlaneSlice = 0
			)
			{
				this->Init(Device, std::forward<D3D12::Descriptor>(Descriptor), Texture, Format, MipSlice, PlaneSlice);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0,
				uint32_t PlaneSlice = 0
			)
			{
				m_Resource.emplace<D3D12::GPUTexture*>(&Texture);
				m_Descriptor = std::move(Descriptor);

				D3D12_RESOURCE_DESC ResourceDesc = Texture.GetResource()->GetDesc();

				D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
				ZeroMemory(&Desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));
				Desc.Format = Format;

				if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
				{
					Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					Desc.Texture2D.MipSlice = MipSlice;
					Desc.Texture2D.PlaneSlice = PlaneSlice;
				}
				else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
				{
					// TODO: Impl
					throw;
				}
				else
				{
					// TODO: Impl
					throw;
				}

				Device->CreateUnorderedAccessView(Texture.GetResource(), nullptr, &Desc, m_Descriptor.GetCPUHandle());
			}
		};

		class ShaderResourceView : public GPUResourceView
		{
		private:

		public:
			ShaderResourceView() = default;

			ShaderResourceView(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
				this->Init(Device, std::forward<D3D12::Descriptor>(Descriptor), Buffer, Format, FirstElement, NumElements, BytesPerElement, TreatAsRawBuffer);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
				m_Resource.emplace<D3D12::GPUBuffer*>(&Buffer);
				m_Descriptor = std::move(Descriptor);

				D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
				ZeroMemory(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
				Desc.Format = Format;
				Desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				Desc.Buffer.FirstElement = FirstElement;
				Desc.Buffer.NumElements = NumElements;
				Desc.Buffer.StructureByteStride = BytesPerElement;
				Desc.Buffer.Flags = TreatAsRawBuffer ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
				Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				Device->CreateShaderResourceView(Buffer.GetResource(), &Desc, m_Descriptor.GetCPUHandle());
			}

			ShaderResourceView(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t NumMipLevels = 1,
				uint32_t MostDetailedMip = 0,
				uint32_t PlaneSlice = 0,
				uint32_t MinAccessibleMipLevel = 0
			)
			{
				this->Init(Device, std::forward<D3D12::Descriptor>(Descriptor), Texture, Format, NumMipLevels, MostDetailedMip, PlaneSlice, MinAccessibleMipLevel);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t NumMipLevels = 1,
				uint32_t MostDetailedMip = 0,
				uint32_t PlaneSlice = 0,
				uint32_t MinAccessibleMipLevel = 0
			)
			{
				m_Resource.emplace<D3D12::GPUTexture*>(&Texture);
				m_Descriptor = std::move(Descriptor);

				D3D12_RESOURCE_DESC ResourceDesc = Texture.GetResource()->GetDesc();

				D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
				ZeroMemory(&Desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
				Desc.Format = Format;
				Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
				{
					Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					Desc.Texture2D.MipLevels = NumMipLevels;
					Desc.Texture2D.MostDetailedMip = MostDetailedMip;
					Desc.Texture2D.PlaneSlice = PlaneSlice;
					Desc.Texture2D.ResourceMinLODClamp = static_cast<FLOAT>(MinAccessibleMipLevel);
				}
				else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
				{
					// TODO: Impl
					throw;
				}
				else
				{
					// TODO: Impl
					throw;
				}

				Device->CreateShaderResourceView(Texture.GetResource(), &Desc, m_Descriptor.GetCPUHandle());
			}
		};

		class DepthStencilView : public GPUResourceView
		{
		private:
			D3D12_CLEAR_VALUE m_ClearValue;

		public:
			DepthStencilView() = default;

			DepthStencilView(DepthStencilView&& Other) noexcept
			{
				GPUResourceView::operator=(std::move(Other));
				m_ClearValue = std::move(Other.m_ClearValue);
			}

			DepthStencilView& operator=(DepthStencilView&& Other) noexcept
			{
				if (this != &Other)
				{
					GPUResourceView::operator=(std::move(Other));
					m_ClearValue = std::move(Other.m_ClearValue);
				}
				return *this;
			}

			DepthStencilView(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				D3D12_CLEAR_VALUE ClearValue,
				uint32_t MipSlice = 0
			)
			{
				this->Init(Device, std::forward<D3D12::Descriptor>(Descriptor), Texture, Format, ClearValue, MipSlice);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				D3D12_CLEAR_VALUE ClearValue,
				uint32_t MipSlice = 0
			)
			{
				m_Resource.emplace<D3D12::GPUTexture*>(&Texture);
				m_ClearValue = std::move(ClearValue);
				m_Descriptor = std::move(Descriptor);

				D3D12_RESOURCE_DESC ResourceDesc = Texture.GetResource()->GetDesc();

				D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
				Desc.Flags = D3D12_DSV_FLAG_NONE;
				Desc.Format = Format;

				if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
				{
					Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					Desc.Texture2D.MipSlice = MipSlice;
				}
				else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
				{
					// TODO: Impl
					throw;
				}
				else
				{
					// TODO: Impl
					throw;
				}

				Device->CreateDepthStencilView(Texture.GetResource(), &Desc, m_Descriptor.GetCPUHandle());
			}

			const D3D12_CLEAR_VALUE& GetClearValue() const { return m_ClearValue; }
		};

		class RenderTargetView : public GPUResourceView
		{
		private:
			D3D12_CLEAR_VALUE m_ClearValue;

		public:
			RenderTargetView() = default;

			RenderTargetView(RenderTargetView&& Other) noexcept
			{
				GPUResourceView::operator=(std::move(Other));
				m_ClearValue = std::move(Other.m_ClearValue);
			}

			RenderTargetView& operator=(RenderTargetView&& Other) noexcept
			{
				if (this != &Other)
				{
					GPUResourceView::operator=(std::move(Other));
					m_ClearValue = std::move(Other.m_ClearValue);
				}
				return *this;
			}

			RenderTargetView(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				D3D12_CLEAR_VALUE ClearValue
			)
			{
				this->Init(Device, std::forward<D3D12::Descriptor>(Descriptor), Texture, Format, ClearValue);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor&& Descriptor,
				GPUTexture& Texture,
				DXGI_FORMAT Format,
				D3D12_CLEAR_VALUE ClearValue,
				uint32_t MipSlice = 0,
				uint32_t PlaneSlice = 0
			)
			{
				m_Resource.emplace<D3D12::GPUTexture*>(&Texture);
				m_ClearValue = std::move(ClearValue);
				m_Descriptor = std::move(Descriptor);

				D3D12_RESOURCE_DESC ResourceDesc = Texture.GetResource()->GetDesc();


				D3D12_RENDER_TARGET_VIEW_DESC Desc;
				Desc.Format = Format;

				if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
				{
					Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					Desc.Texture2D.MipSlice = MipSlice;
					Desc.Texture2D.PlaneSlice = PlaneSlice;
				}
				else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
				{
					// TODO: Impl
					throw;
				}
				else
				{
					// TODO: Impl
					throw;
				}

				Device->CreateRenderTargetView(Texture.GetResource(), &Desc, m_Descriptor.GetCPUHandle());
			}

			const D3D12_CLEAR_VALUE& GetClearValue() const { return m_ClearValue; }
		};
	}
}