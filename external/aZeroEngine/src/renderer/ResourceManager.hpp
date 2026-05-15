#pragma once
#include "graphics_api/resource/buffer/MeshBuffer.hpp"
#include "graphics_api/resource/buffer/IndexedBuffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"
#include "graphics_api/resource/ResourceRecycler.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "FrameContext.hpp"

namespace aZero
{
	namespace Rendering
	{
		// TODO: Impl re-upload to resources
		class ResourceManager
		{
			// TODO: Change to uint16 for relevant things

			struct MaterialData
			{
				uint32_t AlbedoIndex; // Index to descriptor
				uint32_t NormalIndex; // Index to descriptor
			};

		public:
			// Looked up in shader via split batchid
			ResourceManager() = default;

			ResourceManager(ID3D12DeviceX* device, RenderAPI::ResourceRecycler* recycler, RenderAPI::DescriptorHeap& descriptorHeap)
			{
				m_MaterialDataBuffer = RenderAPI::IndexedBuffer<MaterialData>(device, 1000, recycler);
				m_MaterialBufferView = RenderAPI::ShaderResourceView(device, descriptorHeap, m_MaterialDataBuffer.GetBuffer(), 1000, sizeof(MaterialData), 0);
			}

			void UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList, LinearFrameAllocator& frameAllocator, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Mesh& mesh)
			{
				if (mesh.GetRenderID() == Asset::InvalidRenderID) // Doesnt have a render proxy
				{
					aZero::RenderAPI::MeshBuffer MeshBuffer(device, recycler, descriptorHeap, cmdList, mesh.GetVertexData());
					Asset::RenderID renderID = MeshBuffer.GetMeshletsIndex();
					m_MeshMap[mesh.GetAssetID()] = renderID;
					m_MeshBufferMap[renderID] = std::move(MeshBuffer);
					mesh.m_RenderID = renderID; // Set to the MeshBuffer bindless index. The vertex buffer bindless index will always be meshletbindlessindex + 1.
				}
			}

			void UpdateRenderState(LinearFrameAllocator& frameAllocator, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap, Asset::Material& material)
			{
				// TODO: Validate material data
				// TODO: Handle overwriting of the data... defer actual resource destruction until last usage or something...
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
				// TODO: Handle overwriting of the data... defer actual resource destruction until last usage or something...
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
					subresourceData.RowPitch = /*roundUp(*/data.Width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
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
					m_MeshBufferMap.erase(mesh.GetRenderID());
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

			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MeshMap;
			std::unordered_map<Asset::RenderID, RenderAPI::MeshBuffer> m_MeshBufferMap;

			RenderAPI::IndexedBuffer<MaterialData> m_MaterialDataBuffer;
			RenderAPI::ShaderResourceView m_MaterialBufferView;
			std::unordered_map<Asset::AssetID, Asset::RenderID> m_MaterialMap;

			std::unordered_map<Asset::AssetID, RenderAPI::ShaderResourceView> m_TextureDescriptorMap;
			std::unordered_map<Asset::AssetID, RenderAPI::Texture2D> m_TextureMap;

		private:
		};
	}
}