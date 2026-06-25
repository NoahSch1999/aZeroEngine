#pragma once
#include "render_api/resource/buffer/IndexedBuffer.hpp"
#include "render_api/resource/texture/Texture2D.hpp"
#include "render_api/resource/ResourceRecycler.hpp"
#include "render_api/descriptor/DescriptorHeap.hpp"
#include "misc/FreelistAllocator.hpp"
#include "FrameContext.hpp"
#include "assets/Mesh.hpp"
#include "assets/Material.hpp"
#include "assets/Texture.hpp"

namespace aZero
{
	namespace Rendering
	{
		// TODO: Impl re-upload to resources
		class ResourceManager
		{
			struct MaterialData
			{
				// todo Pack as 16bit
				uint32_t AlbedoIndex; // Index to descriptor
				uint32_t NormalIndex; // Index to descriptor
			};

			struct MeshData
			{
				aZero::FreelistAllocator::Allocation MeshletGlobalAllocation;
				aZero::FreelistAllocator::Allocation VertexGlobalAllocation;
			};

		public:
			ResourceManager() = default;

			ResourceManager(ID3D12DeviceX* device, RenderAPI::ResourceRecycler* recycler, RenderAPI::DescriptorHeap& descriptorHeap)
			{
				uint64_t MAX_MESHLETS = 100000;
				uint64_t MAX_VERTICES = 10000000;
				m_MeshletBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_MESHLETS * sizeof(Asset::Meshlet), D3D12_HEAP_TYPE_DEFAULT, false), recycler);
				m_MeshletBoundsBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_MESHLETS * sizeof(DirectX::BoundingSphere), D3D12_HEAP_TYPE_DEFAULT, false), recycler);
				m_PositionBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_VERTICES * sizeof(DXM::Vector3), D3D12_HEAP_TYPE_DEFAULT, false), recycler);
				m_VertexBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_VERTICES * sizeof(Asset::Vertex), D3D12_HEAP_TYPE_DEFAULT, false), recycler);

				m_MeshletFreelist = aZero::FreelistAllocator(MAX_MESHLETS * sizeof(Asset::Meshlet));
				m_VertexFreelist = aZero::FreelistAllocator(MAX_VERTICES * sizeof(Asset::Vertex));

				m_MaterialDataBuffer = RenderAPI::IndexedBuffer<MaterialData>(device, 1000, recycler);
				m_MaterialBufferView = RenderAPI::ShaderResourceView(device, descriptorHeap, m_MaterialDataBuffer.GetBuffer(), 1000, sizeof(MaterialData), 0);
			}

			// todo Use a seperate frameAllocator than the framecontext's when we are loading a lot of meshes at the same time
			void UpdateRenderState(FrameStagingAllocator& frameAllocator, aZero::Asset::Mesh& mesh)
			{
				if (mesh.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					MeshData data{
						.MeshletGlobalAllocation = m_MeshletFreelist.Allocate(mesh.GetVertexData().Meshlets.size() * sizeof(mesh.GetVertexData().Meshlets[0])),
						.VertexGlobalAllocation = m_VertexFreelist.Allocate(mesh.GetVertexData().Vertices.size() * sizeof(mesh.GetVertexData().Vertices[0]))
					};

					frameAllocator.AddAllocation(mesh.GetVertexData().Meshlets.data(), &m_MeshletBuffer, data.MeshletGlobalAllocation.Offset, data.MeshletGlobalAllocation.Size);
					frameAllocator.AddAllocation(mesh.GetVertexData().MeshletBounds.data(), &m_MeshletBoundsBuffer, sizeof(mesh.GetVertexData().MeshletBounds[0]) * (data.MeshletGlobalAllocation.Offset / sizeof(mesh.GetVertexData().Meshlets[0])),
						sizeof(mesh.GetVertexData().MeshletBounds[0]) * (data.MeshletGlobalAllocation.Size / sizeof(mesh.GetVertexData().Meshlets[0])));

					frameAllocator.AddAllocation(mesh.GetVertexData().Vertices.data(), &m_VertexBuffer, data.VertexGlobalAllocation.Offset, data.VertexGlobalAllocation.Size);
					frameAllocator.AddAllocation(mesh.GetVertexData().Positions.data(), 
						&m_PositionBuffer, sizeof(mesh.GetVertexData().Positions[0]) * (data.VertexGlobalAllocation.Offset / sizeof(mesh.GetVertexData().Vertices[0])), 
						sizeof(mesh.GetVertexData().Positions[0]) * (data.VertexGlobalAllocation.Size / sizeof(mesh.GetVertexData().Vertices[0])));

					mesh.m_RenderID = data.MeshletGlobalAllocation.Offset;
					mesh.m_MeshletGlobalOffset = data.MeshletGlobalAllocation.Offset;
					mesh.m_VertexGlobalOffset = data.VertexGlobalAllocation.Offset;
					m_MeshBufferMap_NEW[data.MeshletGlobalAllocation.Offset] = data;
					m_MeshMap_NEW[mesh.GetAssetID()] = data.MeshletGlobalAllocation.Offset;
				}
			}

			void UpdateRenderState(FrameStagingAllocator& frameAllocator, Asset::Material& material)
			{
				if (material.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					material.m_RenderID = m_MaterialDataBuffer.Allocate();
				}

				if (material.GetAlbedoTexture() && material.GetAlbedoTexture()->GetRenderID() == Asset::InvalidRenderID)
				{
					DEBUG_PRINT("Albedo texture isnt set or uploaded to the renderer\n");
				}

				if (material.GetNormalMap() && material.GetNormalMap()->GetRenderID() == Asset::InvalidRenderID)
				{
					DEBUG_PRINT("Normal map isnt set or uploaded to the renderer\n");
				}

				MaterialData data;
				data.AlbedoIndex = material.GetAlbedoTexture() ? material.GetAlbedoTexture()->GetRenderID() : Asset::InvalidRenderID;
				data.NormalIndex = material.GetNormalMap() ? material.GetNormalMap()->GetRenderID() : Asset::InvalidRenderID;
				frameAllocator.AddAllocation(&data, &m_MaterialDataBuffer.GetBuffer(), material.GetRenderID(), sizeof(data));
			}

			void UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Texture& texture)
			{
				// TODO: Validate texture data
				if (texture.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					const auto& data = texture.GetData();
					m_TextureMap[texture.GetAssetID()] = RenderAPI::Texture2D(device, RenderAPI::Texture2D::Desc(data.Width, data.Height, data.Format, D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS), &recycler, {});
					m_TextureDescriptorMap[texture.GetAssetID()] = RenderAPI::ShaderResourceView(device, descriptorHeap, m_TextureMap[texture.GetAssetID()]);
					texture.m_RenderID = m_TextureDescriptorMap[texture.GetAssetID()].GetHeapIndex();

					const uint64_t stagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(m_TextureMap[texture.GetAssetID()].GetResource(), 0, 1));
					RenderAPI::Buffer stagingBuffer(device, RenderAPI::Buffer::Desc(stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD), &recycler);

					D3D12_SUBRESOURCE_DATA subresourceData{};
					subresourceData.pData = data.TexelData.data();
					subresourceData.RowPitch = /*roundUp(*/data.Width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/; // todo Rounding needed?
					subresourceData.SlicePitch = subresourceData.RowPitch * data.Height;

					UpdateSubresources(
						cmdList.Get(),
						m_TextureMap[texture.GetAssetID()].GetResource(),
						stagingBuffer.GetResource(),
						0, 0, 1, &subresourceData);

					m_TextureMap[texture.GetAssetID()].CreateTransition(D3D12_RESOURCE_STATE_COPY_DEST);
					auto barrier = m_TextureMap[texture.GetAssetID()].CreateTransition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
					cmdList->ResourceBarrier(1, &barrier);
				}
			}

			void RemoveRenderState(Asset::Mesh& mesh)
			{
				if (mesh.GetRenderID() != Asset::InvalidRenderID)
				{
					auto alloc = m_MeshBufferMap_NEW[mesh.GetRenderID()];
					m_MeshletFreelist.Free(alloc.MeshletGlobalAllocation);
					m_VertexFreelist.Free(alloc.VertexGlobalAllocation);

					m_MeshBufferMap_NEW.erase(mesh.GetRenderID());
					m_MeshMap_NEW.erase(mesh.GetAssetID());
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

			RenderAPI::Buffer m_MeshletBuffer;
			RenderAPI::Buffer m_MeshletBoundsBuffer;
			aZero::FreelistAllocator m_MeshletFreelist;

			RenderAPI::Buffer m_PositionBuffer;
			RenderAPI::Buffer m_VertexBuffer;
			aZero::FreelistAllocator m_VertexFreelist;

			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MeshMap_NEW;
			std::unordered_map<Asset::RenderID, MeshData> m_MeshBufferMap_NEW;

			RenderAPI::IndexedBuffer<MaterialData> m_MaterialDataBuffer;
			RenderAPI::ShaderResourceView m_MaterialBufferView;
			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MaterialMap;

			std::unordered_map<Asset::AssetID, RenderAPI::ShaderResourceView> m_TextureDescriptorMap;
			std::unordered_map<Asset::AssetID, RenderAPI::Texture2D> m_TextureMap;

		private:
		};
	}
}