#pragma once
#include "GPUResource.hpp"
#include "graphics_api/DescriptorHeap.hpp"

namespace aZero
{
	namespace D3D12
	{
		class GPUResourceView : public NonCopyable
		{
		protected:
			D3D12::Descriptor m_Descriptor;

		public:
			GPUResourceView() = default;

			GPUResourceView(GPUResourceView&& Other) noexcept
			{
				m_Descriptor = std::move(Other.m_Descriptor);
			}

			GPUResourceView& operator=(GPUResourceView&& Other) noexcept
			{
				if (this != &Other)
				{
					m_Descriptor = std::move(Other.m_Descriptor);
				}
				return *this;
			}

			uint32_t GetDescriptorIndex() const { return m_Descriptor.GetHeapIndex(); }
			D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return m_Descriptor.GetCPUHandle(); }
		};

		class UnorderedAccessView : public GPUResourceView
		{
		private:

		public:
			UnorderedAccessView() = default;

			UnorderedAccessView(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
				this->Init(Device, Descriptor, Buffer, Format, FirstElement, NumElements, BytesPerElement, TreatAsRawBuffer);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
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
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0,
				uint32_t PlaneSlice = 0
			)
			{
				this->Init(Device, Descriptor, Texture, Format, MipSlice, PlaneSlice);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0,
				uint32_t PlaneSlice = 0
			)
			{
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
				D3D12::Descriptor& Descriptor,
				const GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
				this->Init(Device, Descriptor, Buffer, Format, FirstElement, NumElements, BytesPerElement, TreatAsRawBuffer);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUBuffer& Buffer,
				DXGI_FORMAT Format,
				uint32_t FirstElement,
				uint32_t NumElements,
				uint32_t BytesPerElement,
				bool TreatAsRawBuffer = false
			)
			{
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
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t NumMipLevels = 1,
				uint32_t MostDetailedMip = 0,
				uint32_t PlaneSlice = 0,
				uint32_t MinAccessibleMipLevel = 0
			)
			{
				this->Init(Device, Descriptor, Texture, Format, NumMipLevels, MostDetailedMip, PlaneSlice, MinAccessibleMipLevel);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t NumMipLevels = 1,
				uint32_t MostDetailedMip = 0,
				uint32_t PlaneSlice = 0,
				uint32_t MinAccessibleMipLevel = 0
			)
			{
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

		public:
			DepthStencilView() = default;

			DepthStencilView(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0
			)
			{
				this->Init(Device, Descriptor, Texture, Format, MipSlice);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0
			)
			{
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
		};

		class RenderTargetView : public GPUResourceView
		{
		private:

		public:
			RenderTargetView() = default;

			RenderTargetView(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format
			)
			{
				this->Init(Device, Descriptor, Texture, Format);
			}

			void Init(
				ID3D12Device* Device,
				D3D12::Descriptor& Descriptor,
				const GPUTexture& Texture,
				DXGI_FORMAT Format,
				uint32_t MipSlice = 0,
				uint32_t PlaneSlice = 0
			)
			{
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
		};
	}
}