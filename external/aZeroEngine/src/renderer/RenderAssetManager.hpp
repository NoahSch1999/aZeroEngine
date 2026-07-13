#pragma once
#include "render_api/resource/buffer/IndexedBuffer.hpp"
#include "render_api/resource/texture/Texture2D.hpp"
#include "render_api/descriptor/DescriptorHeap.hpp"
#include "misc/FreelistAllocator.hpp"
#include "render_api/resource/ResourceRecycler.hpp"
#include "assets/MeshPrimitives.hpp"
#include "FrameStagingAllocator.hpp"

namespace aZero
{
	namespace Rendering
	{
		class Renderer;

		class RenderAssetManager
		{
		public:
			friend class Rendering::Renderer;

			// --------------------------------------------------------------------------------------------------------------
			//			MESH
			// --------------------------------------------------------------------------------------------------------------
			struct MeshAllocation
			{
				aZero::FreelistAllocator::Allocation MeshletGlobalAllocation;
				aZero::FreelistAllocator::Allocation VertexGlobalAllocation;
			};

			// --------------------------------------------------------------------------------------------------------------

			RenderAssetManager() = default;

			RenderAssetManager(ID3D12DeviceX* device, RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap)
			{
				// todo Impl resize
				uint64_t MAX_MESHLETS = 100000;
				uint64_t MAX_VERTICES = 10000000;
				m_MeshletBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_MESHLETS * sizeof(Asset::Meshlet), D3D12_HEAP_TYPE_DEFAULT, false), &recycler);
				m_MeshletBoundsBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_MESHLETS * sizeof(DirectX::BoundingSphere), D3D12_HEAP_TYPE_DEFAULT, false), &recycler);
				m_VertexBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_VERTICES * sizeof(Asset::Vertex), D3D12_HEAP_TYPE_DEFAULT, false), &recycler);

				m_MeshletFreelist = aZero::FreelistAllocator(MAX_MESHLETS * sizeof(Asset::Meshlet));
				m_VertexFreelist = aZero::FreelistAllocator(MAX_VERTICES * sizeof(Asset::Vertex));

				m_MaterialDataBuffer = RenderAPI::IndexedBuffer<aZero::Asset::PBRMaterialData>(device, 1000, &recycler);
				m_MaterialBufferView = RenderAPI::ShaderResourceView(device, descriptorHeap, m_MaterialDataBuffer.GetBuffer(), 1000, sizeof(aZero::Asset::PBRMaterialData), 0);

				// todo Load default assets
				m_DefaultTextureIndex = 0; // set to default index
			}

			// todo Use a seperate frameAllocator than the framecontext's when we are loading a lot of meshes at the same time
			std::pair<uint32_t, uint32_t> UpdateRenderState(FrameStagingAllocator& frameAllocator, const std::vector<Asset::Meshlet>& meshletData, const std::vector<Asset::Vertex>& vertexData, const std::vector<DirectX::BoundingSphere>& meshletBoundsData)
			{
				MeshAllocation meshAlloc{
					.MeshletGlobalAllocation = m_MeshletFreelist.Allocate(meshletData.size() * sizeof(meshletData[0])),
					.VertexGlobalAllocation = m_VertexFreelist.Allocate(vertexData.size() * sizeof(vertexData[0]))
				};

				frameAllocator.AddAllocation(meshletData.data(), &m_MeshletBuffer, meshAlloc.MeshletGlobalAllocation.Offset, meshAlloc.MeshletGlobalAllocation.Size);
				frameAllocator.AddAllocation(meshletBoundsData.data(), &m_MeshletBoundsBuffer, sizeof(meshletBoundsData[0]) * (meshAlloc.MeshletGlobalAllocation.Offset / sizeof(meshletData[0])),
					sizeof(meshletBoundsData[0]) * (meshAlloc.MeshletGlobalAllocation.Size / sizeof(meshletData[0])));

				frameAllocator.AddAllocation(vertexData.data(), &m_VertexBuffer, meshAlloc.VertexGlobalAllocation.Offset, meshAlloc.VertexGlobalAllocation.Size);

				m_MeshBufferMap[meshAlloc.MeshletGlobalAllocation.Offset] = meshAlloc;

				return { meshAlloc.MeshletGlobalAllocation.Offset / sizeof(meshletData[0]), meshAlloc.VertexGlobalAllocation.Offset / sizeof(vertexData[0]) };
			}

			uint32_t UpdateRenderState(FrameStagingAllocator& frameAllocator, uint32_t materialIndex, const aZero::Asset::PBRMaterialData& data)
			{
				uint32_t index = materialIndex;
				if (index == std::numeric_limits<decltype(materialIndex)>::max()) // Create a new entry for the material data if it doesn't have one
				{
					index = m_MaterialDataBuffer.Allocate();
				}

				frameAllocator.AddAllocation(&data, &m_MaterialDataBuffer.GetBuffer(), index * sizeof(data), sizeof(data));

				return index;
			}

			uint32_t UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList,
				RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap,
				const RenderAPI::TextureData& data
			)
			{
				auto descriptor = descriptorHeap.CreateDescriptor();
				uint32_t index = descriptor.GetHeapIndex();

				m_TextureMap[index] = RenderAPI::Texture2D(device, RenderAPI::Texture2D::Desc(data.Height, data.Height, RenderAPI::ToDX_Format(data.Format), D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS, data.MipPitchData.size(), D3D12_RESOURCE_STATE_COMMON), &recycler, {});
				m_TextureDescriptorMap[index] = RenderAPI::ShaderResourceView(device, std::move(descriptor), m_TextureMap[index], data.MipPitchData.size());

				const uint64_t stagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(m_TextureMap[index].GetResource(), 0, data.MipPitchData.size()));
				RenderAPI::Buffer stagingBuffer(device, RenderAPI::Buffer::Desc(stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD), &recycler);

				std::vector<D3D12_SUBRESOURCE_DATA> subresourceData(data.MipPitchData.size());
				for (int32_t mip = 0; mip < data.MipPitchData.size(); mip++)
				{
					subresourceData[mip].pData = data.Data.data() + data.MipPitchData[mip].Offset;
					subresourceData[mip].RowPitch = data.MipPitchData[mip].RowPitch;
					subresourceData[mip].SlicePitch = data.MipPitchData[mip].SlicePitch;
				}

				UpdateSubresources(
					cmdList.Get(),
					m_TextureMap[index].GetResource(),
					stagingBuffer.GetResource(),
					0, 0, data.MipPitchData.size(), subresourceData.data());

				m_TextureMap[index].CreateTransition(D3D12_RESOURCE_STATE_COPY_DEST);
				auto barrier = m_TextureMap[index].CreateTransition(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				cmdList->ResourceBarrier(1, &barrier);
				return index;
			}

			void RemoveMeshAsset(uint32_t id)
			{
				if (auto it = m_MeshBufferMap.find(id); it != m_MeshBufferMap.end())
				{
					m_MeshletFreelist.Free(it->second.MeshletGlobalAllocation);
					m_VertexFreelist.Free(it->second.VertexGlobalAllocation);
					m_MeshBufferMap.erase(it);
				}
			}

			void RemoveMaterialAsset(uint32_t id)
			{
				if (id != std::numeric_limits<uint32_t>::max())
				{
					m_MaterialDataBuffer.Deallocate(id);
				}
			}

			void RemoveTextureAsset(uint32_t id)
			{
				if (m_TextureDescriptorMap.contains(id))
				{
					m_TextureDescriptorMap.erase(id);
					m_TextureMap.erase(id);
				}
			}

			uint32_t GetDefaultTextureIndex() const { return m_DefaultTextureIndex; }

		private:

			// --------------------------------------------------------------------------------------------------------------
			//			MESH
			// --------------------------------------------------------------------------------------------------------------
			RenderAPI::Buffer m_MeshletBuffer;
			RenderAPI::Buffer m_MeshletBoundsBuffer;
			aZero::FreelistAllocator m_MeshletFreelist;

			RenderAPI::Buffer m_VertexBuffer;
			aZero::FreelistAllocator m_VertexFreelist;

			std::unordered_map<uint32_t, MeshAllocation> m_MeshBufferMap; // MeshletGlobalAllocation.Offset is the key

			// --------------------------------------------------------------------------------------------------------------
			//			MATERIAL
			// --------------------------------------------------------------------------------------------------------------
			RenderAPI::IndexedBuffer<aZero::Asset::PBRMaterialData> m_MaterialDataBuffer;
			RenderAPI::ShaderResourceView m_MaterialBufferView;

			// --------------------------------------------------------------------------------------------------------------
			//			TEXTURE2D
			// --------------------------------------------------------------------------------------------------------------
			std::unordered_map<uint32_t, RenderAPI::ShaderResourceView> m_TextureDescriptorMap;
			std::unordered_map<uint32_t, RenderAPI::Texture2D> m_TextureMap;

			// todo Fix fallback texture
			uint32_t m_DefaultTextureIndex = 0;

			// --------------------------------------------------------------------------------------------------------------
		};
	}
}