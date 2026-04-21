#pragma once
#include "DescriptorHeap.hpp"
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class ResourceView
		{
		public:
			ResourceView() = default;
			uint32_t GetHeapIndex() const { return m_Descriptor.GetHeapIndex(); }
			D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_Descriptor.GetCpuHandle(); }
			D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const { return m_Descriptor.GetGpuHandle(); }
			RenderAPI::Descriptor& GetDescriptor() { return m_Descriptor; }
		protected:
			RenderAPI::Descriptor m_Descriptor;
		};

		class ShaderResourceView : public ResourceView
		{
		public:
			ShaderResourceView() = default;
			ShaderResourceView(ID3D12DeviceX* device, DescriptorHeap& heap, const Buffer& resource, uint32_t numElements, uint32_t stride, uint32_t startElement = 0)
			{
				m_Descriptor = heap.CreateDescriptor();
				D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
				desc.Buffer.FirstElement = startElement;
				desc.Buffer.NumElements = numElements;
				desc.Buffer.StructureByteStride = stride;
				desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				device->CreateShaderResourceView(resource.GetResource(), &desc, m_Descriptor.GetCpuHandle());
			}

			ShaderResourceView(ID3D12DeviceX* device, DescriptorHeap& heap, const Texture2D& resource, uint32_t mipLevels = 1, uint32_t mostDetailedMip = 0, uint32_t planeSlice = 0, float resourceMinLODClamp = 0.f)
			{
				m_Descriptor = heap.CreateDescriptor();
				D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
				desc.Texture2D.MipLevels = mipLevels;
				desc.Texture2D.MostDetailedMip = mostDetailedMip;
				desc.Texture2D.PlaneSlice = planeSlice;
				desc.Texture2D.ResourceMinLODClamp = resourceMinLODClamp;
				desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				device->CreateShaderResourceView(resource.GetResource(), &desc, m_Descriptor.GetCpuHandle());
			}
		};

		class UnorderedAccessView : public ResourceView
		{
		public:
			UnorderedAccessView() = default;
			UnorderedAccessView(ID3D12DeviceX* device, DescriptorHeap& heap, const Buffer& resource, uint32_t numElements, uint32_t stride, uint32_t startElement = 0)
			{
				m_Descriptor = heap.CreateDescriptor();
				D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
				desc.Buffer.FirstElement = startElement;
				desc.Buffer.NumElements = numElements;
				desc.Buffer.StructureByteStride = stride;
				desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				device->CreateUnorderedAccessView(resource.GetResource(), nullptr, &desc, m_Descriptor.GetCpuHandle());
			}
		};

		class RenderTargetView : public ResourceView
		{
		public:
			RenderTargetView() = default;
			RenderTargetView(ID3D12DeviceX* device, DescriptorHeap& heap, const Texture2D& resource, DXGI_FORMAT format, uint32_t mipSlice = 0, uint32_t planeSlice = 0)
			{
				m_Descriptor = heap.CreateDescriptor();
				D3D12_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = format;
				desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = mipSlice;
				desc.Texture2D.PlaneSlice = planeSlice;

				device->CreateRenderTargetView(resource.GetResource(), &desc, m_Descriptor.GetCpuHandle());
			}
		};

		class DepthStencilTargetView : public ResourceView
		{
		public:
			DepthStencilTargetView() = default;
			DepthStencilTargetView(ID3D12DeviceX* device, DescriptorHeap& heap, const Texture2D& resource, DXGI_FORMAT format, uint32_t mipSlice = 0)
			{
				m_Descriptor = heap.CreateDescriptor();
				D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
				desc.Format = format;
				desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MipSlice = mipSlice;

				device->CreateDepthStencilView(resource.GetResource(), &desc, m_Descriptor.GetCpuHandle());
			}
		};
	}
}