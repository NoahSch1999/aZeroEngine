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
		class MeshletBuffer
		{
		public:
			MeshletBuffer() = default;
			MeshletBuffer(ID3D12DeviceX* device, ResourceRecycler& resourceRecycler, DescriptorHeap& heap, CommandList& cmdList, const Asset::MeshletMeshData& data)
			{
				// TODO: Make all of these DEFAULT heap types
				// TODO: Make these work with bindless instead of binding them via virtual addresses

				Buffer::Desc desc;
				desc.AccessType = D3D12_HEAP_TYPE_DEFAULT;

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Buffer.FirstElement = 0;
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				desc.NumBytes = data.Positions.size() * sizeof(Asset::VertexPosition);
				m_PositionBuffer = Buffer(device, desc, &resourceRecycler);
				m_PositionDescriptor = ShaderResourceView(device, heap, m_PositionBuffer, data.Positions.size(), sizeof(Asset::VertexPosition));

				desc.NumBytes = data.GenericVertexData.size() * sizeof(Asset::GenericVertexData);
				m_GenericVertexBuffer = Buffer(device, desc, &resourceRecycler);
				m_GenericVertexDescriptor = ShaderResourceView(device, heap, m_GenericVertexBuffer, data.GenericVertexData.size(), sizeof(Asset::GenericVertexData));

				desc.NumBytes = data.Meshlets.size() * sizeof(Asset::Meshlet);
				m_MeshletBuffer = Buffer(device, desc, &resourceRecycler);
				m_MeshletDescriptor = ShaderResourceView(device, heap, m_MeshletBuffer, data.Meshlets.size(), sizeof(Asset::Meshlet));

				desc.NumBytes = data.MeshletIndices.size() * sizeof(Asset::VertexIndex);
				m_IndicesBuffer = Buffer(device, desc, &resourceRecycler);
				m_IndicesDescriptor = ShaderResourceView(device, heap, m_IndicesBuffer, data.MeshletIndices.size(), sizeof(Asset::VertexIndex));

				desc.NumBytes = data.MeshletPrimitives.size() * sizeof(uint8_t);
				m_PrimitivesBuffer = Buffer(device, desc, &resourceRecycler);
				m_PrimitivesDescriptor = ShaderResourceView(device, heap, m_PrimitivesBuffer, data.MeshletPrimitives.size(), sizeof(uint8_t), 0);

				Buffer::Desc stagingDesc;
				stagingDesc.AccessType = D3D12_HEAP_TYPE_UPLOAD;
				stagingDesc.NumBytes = 
					  data.Positions.size() * sizeof(Asset::VertexPosition)
					+ data.GenericVertexData.size() * sizeof(Asset::GenericVertexData)
					+ data.Meshlets.size() * sizeof(Asset::Meshlet)
					+ data.MeshletIndices.size() * sizeof(Asset::VertexIndex)
					+ data.MeshletPrimitives.size() * sizeof(uint8_t);

				RenderAPI::Buffer stagingBuffer(device, stagingDesc, &resourceRecycler);

				uint32_t offset = 0;
				stagingBuffer.Write(data.Positions.data(), data.Positions.size() * sizeof(Asset::VertexPosition), offset);
				cmdList->CopyBufferRegion(m_PositionBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Positions.size() * sizeof(Asset::VertexPosition));
				offset += data.Positions.size() * sizeof(Asset::VertexPosition);

				stagingBuffer.Write(data.GenericVertexData.data(), data.GenericVertexData.size() * sizeof(Asset::GenericVertexData), offset);
				cmdList->CopyBufferRegion(m_GenericVertexBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.GenericVertexData.size() * sizeof(Asset::GenericVertexData));
				offset += data.GenericVertexData.size() * sizeof(Asset::GenericVertexData);

				stagingBuffer.Write(data.Meshlets.data(), data.Meshlets.size() * sizeof(Asset::Meshlet), offset);
				cmdList->CopyBufferRegion(m_MeshletBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.Meshlets.size() * sizeof(Asset::Meshlet));
				offset += data.Meshlets.size() * sizeof(Asset::Meshlet);

				stagingBuffer.Write(data.MeshletIndices.data(), data.MeshletIndices.size() * sizeof(Asset::VertexIndex), offset);
				cmdList->CopyBufferRegion(m_IndicesBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.MeshletIndices.size() * sizeof(Asset::VertexIndex));
				offset += data.MeshletIndices.size() * sizeof(Asset::VertexIndex);

				stagingBuffer.Write(data.MeshletPrimitives.data(), data.MeshletPrimitives.size() * sizeof(uint8_t), offset);
				cmdList->CopyBufferRegion(m_PrimitivesBuffer.GetResource(), 0, stagingBuffer.GetResource(), offset, data.MeshletPrimitives.size() * sizeof(uint8_t));
			}

			uint32_t GetPositionsIndex() const { return m_PositionDescriptor.GetHeapIndex(); }
			uint32_t GetGenericVertexDataIndex() const { return m_GenericVertexDescriptor.GetHeapIndex(); }
			uint32_t GetMeshletsIndex() const { return m_MeshletDescriptor.GetHeapIndex(); }
			uint32_t GetIndicesIndex() const { return m_IndicesDescriptor.GetHeapIndex(); }
			uint32_t GetPrimitivesIndex() const { return m_PrimitivesDescriptor.GetHeapIndex(); }

		private:
			ShaderResourceView m_PositionDescriptor;
			Buffer m_PositionBuffer;

			ShaderResourceView m_GenericVertexDescriptor;
			Buffer m_GenericVertexBuffer;

			ShaderResourceView m_MeshletDescriptor;
			Buffer m_MeshletBuffer;

			ShaderResourceView m_IndicesDescriptor;
			Buffer m_IndicesBuffer;

			ShaderResourceView m_PrimitivesDescriptor;
			Buffer m_PrimitivesBuffer;
		};

		//class VertexBuffer
		//{
		//private:
		//	struct DescriptorBufferCombo
		//	{
		//		Descriptor m_Descriptor;
		//		Buffer m_Buffer;

		//		DescriptorBufferCombo() = default;

		//		template<typename T>
		//		void Init(ID3D12DeviceX* device, DescriptorHeap& heap, ResourceRecycler& resourceRecycler, const std::vector<T>& data, RenderAPI::CommandList& stagingCmdList)
		//		{
		//			Buffer::Desc desc(data.size() * sizeof(T), D3D12_HEAP_TYPE_DEFAULT);
		//			m_Buffer = Buffer(device, desc, &resourceRecycler); 
		//			
		//			desc.AccessType = D3D12_HEAP_TYPE_UPLOAD;
		//			Buffer uploadBuffer(device, desc, &resourceRecycler);

		//			memcpy((char*)uploadBuffer.GetCPUAccessibleMemory(), data.data(), desc.NumBytes);
		//			stagingCmdList->CopyResource(m_Buffer.GetResource(), uploadBuffer.GetResource());
		//			m_Descriptor = heap.CreateDescriptor();

		//			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		//			ZeroMemory(&srvDesc, sizeof(srvDesc));

		//			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		//			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

		//			srvDesc.Buffer.FirstElement = 0;
		//			srvDesc.Buffer.NumElements = static_cast<UINT>(data.size());
		//			srvDesc.Buffer.StructureByteStride = sizeof(data.at(0));

		//			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//			device->CreateShaderResourceView(m_Buffer.GetResource(), &srvDesc, m_Descriptor.GetCpuHandle());
		//		}
		//	};

		//public:
		//	DescriptorBufferCombo m_Positions;
		//	DescriptorBufferCombo m_UVs;
		//	DescriptorBufferCombo m_Normals;
		//	DescriptorBufferCombo m_Tangents;
		//	DescriptorBufferCombo m_Indices;

		//	VertexBuffer() = default;
		//	VertexBuffer(
		//		ID3D12DeviceX* device,
		//		RenderAPI::CommandList& uploadCommandList,
		//		ResourceRecycler& resourceRecycler,
		//		const Asset::Mesh::Data& data,
		//		DescriptorHeap& heap
		//	)
		//	{
		//		if (data.Positions.empty() && data.UVs.empty() && data.Normals.empty() && data.Tangents.empty() && data.Indices.empty())
		//		{
		//			throw std::runtime_error("One or more vertex channels doesn't have any data.");
		//		}

		//		// todo Rework so they are sharing the same resource and use the same stagingbuffer
		//		m_Positions.Init(device, heap, resourceRecycler, data.Positions, uploadCommandList);
		//		m_UVs.Init(device, heap, resourceRecycler, data.UVs, uploadCommandList);
		//		m_Normals.Init(device, heap, resourceRecycler, data.Normals, uploadCommandList);
		//		m_Tangents.Init(device, heap, resourceRecycler, data.Tangents, uploadCommandList);
		//		m_Indices.Init(device, heap, resourceRecycler, data.Indices, uploadCommandList);
		//	}
		//};
	}
}