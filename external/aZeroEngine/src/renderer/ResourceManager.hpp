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
			}

			void UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList, LinearFrameAllocator& frameAllocator, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Mesh& mesh)
			{
				// TODO: Validate mesh data
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

				if (material.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					material.m_RenderID = m_MaterialDataBuffer.Allocate();
				}

				if (material.GetAlbedoTexture()->GetRenderID() == Asset::InvalidRenderID)
				{
					this->UpdateRenderState(recycler, descriptorHeap, *material.GetAlbedoTexture());
				}

				if (material.GetNormalMap()->GetRenderID() == Asset::InvalidRenderID)
				{
					this->UpdateRenderState(recycler, descriptorHeap, *material.GetNormalMap());
				}

				// TODO: Fill in m_MaterialDataBuffer
				MaterialData data;
				data.AlbedoIndex = material.GetAlbedoTexture()->GetRenderID();
				data.NormalIndex = material.GetNormalMap()->GetRenderID();
				//frameAllocator.AddAllocation(&data, &m_MaterialDataBuffer.GetBuffer(), material.GetRenderID(), sizeof(data));
			}

			void UpdateRenderState(RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Texture& texture)
			{
				// TODO: Validate texture data
				if (texture.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					m_TextureDescriptorMap[texture.GetAssetID()] = descriptorHeap.CreateDescriptor();
					texture.m_RenderID = m_TextureDescriptorMap[texture.GetAssetID()].GetHeapIndex();
				}

				// TODO: Create the m_TextureMap resource and descriptor
				
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
			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MaterialMap;

			std::unordered_map<Asset::AssetID, RenderAPI::Descriptor> m_TextureDescriptorMap;
			std::unordered_map<Asset::AssetID, RenderAPI::Texture2D> m_TextureMap;

		private:
		};
	}
}