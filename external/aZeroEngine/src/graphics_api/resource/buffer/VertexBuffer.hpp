#pragma once
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "graphics_api/command_recording/CommandList.hpp"
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "Assets/Mesh.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class VertexBuffer
		{
		private:
			struct DescriptorBufferCombo
			{
				Descriptor m_Descriptor;
				Buffer m_Buffer;

				DescriptorBufferCombo() = default;

				template<typename T>
				void Init(ID3D12DeviceX* device, DescriptorHeap& heap, ResourceRecycler& resourceRecycler, const std::vector<T>& data, RenderAPI::CommandList& stagingCmdList)
				{
					Buffer::Desc desc(data.size() * sizeof(T), D3D12_HEAP_TYPE_DEFAULT);
					m_Buffer.Init(device, desc, &resourceRecycler); 
					
					desc.AccessType = D3D12_HEAP_TYPE_UPLOAD;
					Buffer uploadBuffer(device, desc, &resourceRecycler);

					memcpy((char*)uploadBuffer.GetCPUAccessibleMemory(), data.data(), desc.NumBytes);
					stagingCmdList->CopyResource(m_Buffer.GetResource(), uploadBuffer.GetResource());
					m_Descriptor = heap.CreateDescriptor();

					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
					ZeroMemory(&srvDesc, sizeof(srvDesc));

					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

					srvDesc.Buffer.FirstElement = 0;
					srvDesc.Buffer.NumElements = static_cast<UINT>(data.size());
					srvDesc.Buffer.StructureByteStride = sizeof(data.at(0));

					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					device->CreateShaderResourceView(m_Buffer.GetResource(), &srvDesc, m_Descriptor.GetCpuHandle());
				}
			};

		public:
			DescriptorBufferCombo m_Positions;
			DescriptorBufferCombo m_UVs;
			DescriptorBufferCombo m_Normals;
			DescriptorBufferCombo m_Tangents;
			DescriptorBufferCombo m_Indices;

			VertexBuffer() = default;
			VertexBuffer(
				ID3D12DeviceX* device,
				RenderAPI::CommandList& uploadCommandList,
				ResourceRecycler& resourceRecycler,
				const Asset::Mesh::Data& data,
				DescriptorHeap& heap
			)
			{
				if (data.Positions.empty() && data.UVs.empty() && data.Normals.empty() && data.Tangents.empty() && data.Indices.empty())
				{
					throw std::runtime_error("One or more vertex channels doesn't have any data.");
				}

				// todo Rework so they are sharing the same resource and use the same stagingbuffer
				m_Positions.Init(device, heap, resourceRecycler, data.Positions, uploadCommandList);
				m_UVs.Init(device, heap, resourceRecycler, data.UVs, uploadCommandList);
				m_Normals.Init(device, heap, resourceRecycler, data.Normals, uploadCommandList);
				m_Tangents.Init(device, heap, resourceRecycler, data.Tangents, uploadCommandList);
				m_Indices.Init(device, heap, resourceRecycler, data.Indices, uploadCommandList);
			}
		};
	}
}