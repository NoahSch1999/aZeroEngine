#pragma once
#include "graphics_api/resource/buffer/VertexBuffer.hpp"
#include "graphics_api/resource/buffer/IndexedBuffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"
#include "scene/Scene.hpp"
#include "graphics_api/resource/ResourceRecycler.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "FrameContext.hpp"

namespace aZero
{
	namespace Rendering
	{
		class ResourceManager
		{

			struct MaterialData
			{
				uint32_t AlbedoIndex; // Index to descriptor
				uint32_t NormalIndex; // Index to descriptor
			};

			struct GPUMesh
			{
				uint32_t MeshletCount;
				uint32_t MeshletBuffer;
				uint32_t PrimitiveBuffer;
				uint32_t IndicesBuffer;
				uint32_t PositionBuffer;
				uint32_t VertexDataBuffer;
				DirectX::BoundingSphere Bounds;
			};

		public:
			// Looked up in shader via split batchid
			ResourceManager() = default;

			ResourceManager(ID3D12DeviceX* device, RenderAPI::ResourceRecycler* recycler, RenderAPI::DescriptorHeap& descriptorHeap)
			{
				m_MeshDataBuffer = RenderAPI::IndexedBuffer<GPUMesh>(device, 1000, recycler);
				m_MeshBufferView = RenderAPI::ShaderResourceView(device, descriptorHeap, m_MeshDataBuffer.GetBuffer(), 1000, sizeof(GPUMesh), 0);

				m_MaterialDataBuffer = RenderAPI::IndexedBuffer<MaterialData>(device, 1000, recycler);
				m_MaterialBufferView = RenderAPI::ShaderResourceView(device, descriptorHeap, m_MaterialDataBuffer.GetBuffer(), 1000, sizeof(MaterialData), 0);
			}

			void UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList, LinearFrameAllocator& frameAllocator, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Mesh& mesh)
			{
				// TODO: Validate mesh data
				// TODO: Handle overwriting of the data... defer actual resource destruction until last usage or something...
				if (mesh.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					mesh.m_RenderID = m_MeshDataBuffer.Allocate();
				}

				auto& data = mesh.GetVertexData();
				m_MeshletBufferMap[mesh.GetRenderID()] = aZero::RenderAPI::MeshletBuffer(device, recycler, descriptorHeap, cmdList, data);
				m_MeshMap[mesh.GetAssetID()] = mesh.GetRenderID();

				auto& meshletBuffer = m_MeshletBufferMap[mesh.GetAssetID()];

				GPUMesh gpuMesh;
				gpuMesh.Bounds = data.Bounds;
				gpuMesh.IndicesBuffer = meshletBuffer.GetIndicesIndex();
				gpuMesh.MeshletBuffer = meshletBuffer.GetMeshletsIndex();
				gpuMesh.PositionBuffer = meshletBuffer.GetPositionsIndex();
				gpuMesh.PrimitiveBuffer = meshletBuffer.GetPrimitivesIndex();
				gpuMesh.VertexDataBuffer = meshletBuffer.GetGenericVertexDataIndex();
				gpuMesh.MeshletCount = data.Meshlets.size();

				frameAllocator.AddAllocation(&gpuMesh, &m_MeshDataBuffer.GetBuffer(), mesh.GetRenderID() * sizeof(gpuMesh), sizeof(gpuMesh));
			}

			void UpdateRenderState(LinearFrameAllocator& frameAllocator, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Material& material)
			{
				// TODO: Validate material data
				// TODO: Handle overwriting of the data... defer actual resource destruction until last usage or something...
				if (material.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					material.m_RenderID = m_MaterialDataBuffer.Allocate();
				}

				if (material.GetAlbedoTexture()->GetRenderID() == Asset::InvalidRenderID)
				{
					DEBUG_PRINT("Albedo texture isnt uploaded to the renderer\n");
					return;
				}

				if (material.GetNormalMap()->GetRenderID() == Asset::InvalidRenderID)
				{
					DEBUG_PRINT("Normal map isnt uploaded to the renderer\n");
					return;
				}

				MaterialData data;
				data.AlbedoIndex = material.GetAlbedoTexture()->GetRenderID();
				data.NormalIndex = material.GetNormalMap()->GetRenderID();
				frameAllocator.AddAllocation(&data, &m_MaterialDataBuffer.GetBuffer(), material.GetRenderID(), sizeof(data));
			}

			void UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Texture& texture)
			{
				// TODO: Validate texture data
				// TODO: Handle overwriting of the data... defer actual resource destruction until last usage or something...
				if (texture.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					const auto& data = texture.GetData();
					m_TextureMap[texture.GetAssetID()] = RenderAPI::Texture2D(device, RenderAPI::Texture2D::Desc(data.Width, data.Height, data.Format, D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST), &recycler, {});
					m_TextureDescriptorMap[texture.GetAssetID()] = RenderAPI::ShaderResourceView(device, descriptorHeap, m_TextureMap[texture.GetAssetID()]);
					texture.m_RenderID = m_TextureDescriptorMap[texture.GetAssetID()].GetHeapIndex();

					const uint64_t stagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(m_TextureMap[texture.GetAssetID()].GetResource(), 0, 1));
					RenderAPI::Buffer stagingBuffer(device, RenderAPI::Buffer::Desc(stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD), &recycler);

					D3D12_SUBRESOURCE_DATA subresourceData{};
					subresourceData.pData = data.TexelData.data();
					subresourceData.RowPitch = /*roundUp(*/data.Width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
					subresourceData.SlicePitch = subresourceData.RowPitch * data.Height;

					UpdateSubresources(
						cmdList.Get(),
						m_TextureMap[texture.GetAssetID()].GetResource(),
						stagingBuffer.GetResource(),
						0, 0, 1, &subresourceData);

					auto barrier = m_TextureMap[texture.GetAssetID()].CreateTransition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
					cmdList->ResourceBarrier(1, &barrier);
				}
			}

			void RemoveRenderState(Asset::Mesh& mesh)
			{
				if (mesh.GetRenderID() != Asset::InvalidRenderID)
				{
					m_MeshDataBuffer.Deallocate(mesh.GetRenderID()); // OK to remove this instantly?
					m_MeshletBufferMap.erase(mesh.GetAssetID());
					m_MeshMap.erase(mesh.GetAssetID());
					mesh.m_RenderID = Asset::InvalidRenderID;
				}
			}

			void RemoveRenderState(Asset::Material& material)
			{
				if (material.GetRenderID() != Asset::InvalidRenderID)
				{
					m_MaterialDataBuffer.Deallocate(material.GetRenderID());
					m_MaterialMap.erase(material.GetAssetID());
					material.m_RenderID = Asset::InvalidRenderID;
				}
			}

			void RemoveRenderState(Asset::Texture& texture)
			{
				if (texture.GetRenderID() != Asset::InvalidRenderID)
				{
					m_TextureDescriptorMap.erase(texture.GetAssetID()); // OK to remove this instantly?
					m_TextureMap.erase(texture.GetAssetID());
					texture.m_RenderID = Asset::InvalidRenderID;
				}
			}

			/*
			I WANT: 
				A buffer that can be indexed into
				Has max size of max meshes/materials
			*/
			RenderAPI::IndexedBuffer<GPUMesh> m_MeshDataBuffer;
			RenderAPI::ShaderResourceView m_MeshBufferView;
			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MeshMap;
			std::unordered_map<Asset::RenderID, RenderAPI::MeshletBuffer> m_MeshletBufferMap;

			RenderAPI::IndexedBuffer<MaterialData> m_MaterialDataBuffer;
			RenderAPI::ShaderResourceView m_MaterialBufferView;
			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MaterialMap;

			std::unordered_map<Asset::AssetID, RenderAPI::ShaderResourceView> m_TextureDescriptorMap;
			std::unordered_map<Asset::AssetID, RenderAPI::Texture2D> m_TextureMap;

		private:
		};
	}
}