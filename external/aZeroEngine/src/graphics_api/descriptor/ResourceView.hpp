#pragma once
#include "Descriptor.hpp"
#include "graphics_api/resource/buffer/Buffer.hpp"

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
		};
	}
}