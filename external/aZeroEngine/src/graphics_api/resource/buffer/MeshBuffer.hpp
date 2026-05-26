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

				std::vector<Descriptor> descriptors = heap.CreateContiguousDescriptors(4);

				desc.NumBytes = data.Meshlets.size() * sizeof(data.Meshlets[0]);
				m_MeshBuffer = Buffer(device, desc, &resourceRecycler);
				m_MeshletDescriptor = ShaderResourceView(device, std::move(descriptors[0]), m_MeshBuffer, data.Meshlets.size(), sizeof(data.Meshlets[0]));
				std::string strN = "Meshletbuffer";
				std::wstring str(strN.begin(), strN.end());
				m_MeshBuffer.GetResource()->SetName(str.c_str());

				desc.NumBytes = data.Primitives.size() * sizeof(data.Primitives[0]);
				m_PrimitivesBuffer = Buffer(device, desc, &resourceRecycler);
				m_PrimitivesDescriptor = ShaderResourceView(device, std::move(descriptors[1]), m_PrimitivesBuffer, data.Primitives.size(), sizeof(data.Primitives[0]));
				strN = "Primitivebuffer";
				str.assign(strN.begin(), strN.end());
				m_PrimitivesBuffer.GetResource()->SetName(str.c_str());

				desc.NumBytes = data.Vertices.size() * sizeof(data.Vertices[0]);
				m_VerticesBuffer = Buffer(device, desc, &resourceRecycler);
				m_VerticesDescriptor = ShaderResourceView(device, std::move(descriptors[2]), m_VerticesBuffer, data.Vertices.size(), sizeof(data.Vertices[0]));
				strN = "VertexBuffer";
				str.assign(strN.begin(), strN.end());
				m_VerticesBuffer.GetResource()->SetName(str.c_str());

				desc.NumBytes = data.Indices.size() * sizeof(data.Indices[0]);
				m_IndicesBuffer = Buffer(device, desc, &resourceRecycler);
				m_IndicesDescriptor = ShaderResourceView(device, std::move(descriptors[3]), m_IndicesBuffer, data.Indices.size(), sizeof(data.Indices[0]));
				strN = "IndicesBuffer";
				str.assign(strN.begin(), strN.end());
				m_IndicesBuffer.GetResource()->SetName(str.c_str());

				Buffer::Desc stagingDesc;
				stagingDesc.AccessType = D3D12_HEAP_TYPE_UPLOAD;
				stagingDesc.NumBytes =
					data.Meshlets.size() * sizeof(data.Meshlets[0])
					+ data.Primitives.size() * sizeof(data.Primitives[0])
					+ data.Vertices.size() * sizeof(data.Vertices[0])
					+ data.Indices.size() * sizeof(data.Indices[0])
					;

				RenderAPI::Buffer stagingBuffer(device, stagingDesc, &resourceRecycler);

				uint32_t offset = 0;

				stagingBuffer.Write(data.Meshlets.data(), data.Meshlets.size() * sizeof(data.Meshlets[0]), offset);
				cmdList->CopyBufferRegion(m_MeshBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Meshlets.size() * sizeof(data.Meshlets[0]));
				offset += data.Meshlets.size() * sizeof(data.Meshlets[0]);

				stagingBuffer.Write(data.Primitives.data(), data.Primitives.size() * sizeof(data.Primitives[0]), offset);
				cmdList->CopyBufferRegion(m_PrimitivesBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Primitives.size() * sizeof(data.Primitives[0]));
				offset += data.Primitives.size() * sizeof(data.Primitives[0]);

				stagingBuffer.Write(data.Vertices.data(), data.Vertices.size() * sizeof(data.Vertices[0]), offset);
				cmdList->CopyBufferRegion(m_VerticesBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Vertices.size() * sizeof(data.Vertices[0]));
				offset += data.Vertices.size() * sizeof(data.Vertices[0]);

				stagingBuffer.Write(data.Indices.data(), data.Indices.size() * sizeof(data.Indices[0]), offset);
				cmdList->CopyBufferRegion(m_IndicesBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Indices.size() * sizeof(data.Indices[0]));
				offset += data.Indices.size() * sizeof(data.Indices[0]);
				
			}

			uint32_t GetMeshletsIndex() const { return m_MeshletDescriptor.GetHeapIndex(); }
			uint32_t GetPrimitivesIndex() const { return m_PrimitivesDescriptor.GetHeapIndex(); }
			uint32_t GetVerticesIndex() const { return m_VerticesDescriptor.GetHeapIndex(); }

		private:
			ShaderResourceView m_MeshletDescriptor;
			Buffer m_MeshBuffer;
			ShaderResourceView m_PrimitivesDescriptor;
			Buffer m_PrimitivesBuffer;
			ShaderResourceView m_VerticesDescriptor;
			Buffer m_VerticesBuffer;
			ShaderResourceView m_IndicesDescriptor;
			Buffer m_IndicesBuffer;
		};
	}
}