#pragma once
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "graphics_api/command_recording/CommandList.hpp"
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "Assets/Mesh.hpp"
#include "graphics_api/descriptor/ResourceView.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		// TODO: Cleanup and make more "correct"
		class MeshBuffer
		{
		public:
			MeshBuffer() = default;
			MeshBuffer(ID3D12DeviceX* device, ResourceRecycler& resourceRecycler, DescriptorHeap& heap, CommandList& cmdList, const Asset::MeshletMeshData& data)
			{
				Buffer::Desc desc;
				desc.AccessType = D3D12_HEAP_TYPE_DEFAULT;

				std::vector<Descriptor> descriptors = heap.CreateContiguousDescriptors(2);

				desc.NumBytes = data.Meshlets.size() * sizeof(Asset::Meshlet);
				m_MeshBuffer = Buffer(device, desc, &resourceRecycler);
				m_MeshletDescriptor = ShaderResourceView(device, std::move(descriptors[0]), m_MeshBuffer, data.Meshlets.size(), sizeof(Asset::Meshlet));
				std::string strN = "Meshletbuffer: " + data.Name;
				std::wstring str(strN.begin(), strN.end());
				m_MeshBuffer.GetResource()->SetName(str.c_str());

				desc.NumBytes = data.Vertices.size() * sizeof(Asset::Vertex);
				m_VerticesBuffer = Buffer(device, desc, &resourceRecycler);
				m_VerticesDescriptor = ShaderResourceView(device, std::move(descriptors[1]), m_VerticesBuffer, data.Vertices.size(), sizeof(Asset::Vertex));
				strN = "VertexBuffer: " + data.Name;
				str.assign(strN.begin(), strN.end());
				m_VerticesBuffer.GetResource()->SetName(str.c_str());

				Buffer::Desc stagingDesc;
				stagingDesc.AccessType = D3D12_HEAP_TYPE_UPLOAD;
				stagingDesc.NumBytes = 
					  data.Vertices.size() * sizeof(Asset::Vertex)
					+ data.Meshlets.size() * sizeof(Asset::Meshlet);

				RenderAPI::Buffer stagingBuffer(device, stagingDesc, &resourceRecycler);

				uint32_t offset = 0;
				stagingBuffer.Write(data.Vertices.data(), data.Vertices.size() * sizeof(Asset::Vertex), offset);
				cmdList->CopyBufferRegion(m_VerticesBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Vertices.size() * sizeof(Asset::Vertex));
				offset += data.Vertices.size() * sizeof(Asset::Vertex);
				stagingBuffer.Write(data.Meshlets.data(), data.Meshlets.size() * sizeof(Asset::Meshlet), offset);
				cmdList->CopyBufferRegion(m_MeshBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Meshlets.size() * sizeof(Asset::Meshlet));
				offset += data.Meshlets.size() * sizeof(Asset::Meshlet);
			}

			uint32_t GetMeshletsIndex() const { return m_MeshletDescriptor.GetHeapIndex(); }
			uint32_t GetVerticesIndex() const { return m_VerticesDescriptor.GetHeapIndex(); }

		private:
			ShaderResourceView m_VerticesDescriptor;
			Buffer m_VerticesBuffer;
			ShaderResourceView m_MeshletDescriptor;
			Buffer m_MeshBuffer;
		};
	}
}