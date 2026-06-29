#pragma once
#include "render_api/resource/buffer/IndexedBuffer.hpp"
#include "render_api/resource/texture/Texture2D.hpp"
#include "render_api/descriptor/DescriptorHeap.hpp"
#include "misc/FreelistAllocator.hpp"
#include "render_api/resource/ResourceRecycler.hpp"
#include "assets/MeshPrimitives.hpp"

namespace aZero
{
	namespace Rendering
	{
		class RenderAssetManager
		{
		public:
			// --------------------------------------------------------------------------------------------------------------
			//			MESH
			// --------------------------------------------------------------------------------------------------------------
			struct MeshAllocation
			{
				aZero::FreelistAllocator::Allocation MeshletGlobalAllocation;
				aZero::FreelistAllocator::Allocation VertexGlobalAllocation;
			};

			// --------------------------------------------------------------------------------------------------------------
			//			MATERIAL
			// --------------------------------------------------------------------------------------------------------------

			struct MaterialData
			{
				uint32_t AlbedoDescriptorIndex = std::numeric_limits<uint32_t>::max();
				uint32_t NormalDescriptorIndex = std::numeric_limits<uint32_t>::max();
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
				m_PositionBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_VERTICES * sizeof(DXM::Vector3), D3D12_HEAP_TYPE_DEFAULT, false), &recycler);
				m_VertexBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(MAX_VERTICES * sizeof(Asset::Vertex), D3D12_HEAP_TYPE_DEFAULT, false), &recycler);

				m_MeshletFreelist = aZero::FreelistAllocator(MAX_MESHLETS * sizeof(Asset::Meshlet));
				m_VertexFreelist = aZero::FreelistAllocator(MAX_VERTICES * sizeof(Asset::Vertex));

				m_MaterialDataBuffer = RenderAPI::IndexedBuffer<MaterialData>(device, 1000, &recycler);
				m_MaterialBufferView = RenderAPI::ShaderResourceView(device, descriptorHeap, m_MaterialDataBuffer.GetBuffer(), 1000, sizeof(MaterialData), 0);

				// todo Load default assets
				m_DefaultTextureIndex = 0; // set to default index
			}

			// todo Use a seperate frameAllocator than the framecontext's when we are loading a lot of meshes at the same time
			std::pair<uint32_t, uint32_t> UpdateRenderState(FrameStagingAllocator& frameAllocator, 
				const std::vector<Asset::Meshlet>& meshletData, const std::vector<Asset::Vertex>& vertexData, 
				const std::vector<DXM::Vector3>& positionData, const std::vector<DirectX::BoundingSphere>& meshletBoundsData)
			{
				MeshAllocation meshAlloc{
					.MeshletGlobalAllocation = m_MeshletFreelist.Allocate(meshletData.size() * sizeof(meshletData[0])),
					.VertexGlobalAllocation = m_VertexFreelist.Allocate(vertexData.size() * sizeof(vertexData[0]))
				};

				frameAllocator.AddAllocation(meshletData.data(), &m_MeshletBuffer, meshAlloc.MeshletGlobalAllocation.Offset, meshAlloc.MeshletGlobalAllocation.Size);
				frameAllocator.AddAllocation(meshletBoundsData.data(), &m_MeshletBoundsBuffer, sizeof(meshletBoundsData[0]) * (meshAlloc.MeshletGlobalAllocation.Offset / sizeof(meshletData[0])),
					sizeof(meshletBoundsData[0]) * (meshAlloc.MeshletGlobalAllocation.Size / sizeof(meshletData[0])));

				frameAllocator.AddAllocation(vertexData.data(), &m_VertexBuffer, meshAlloc.VertexGlobalAllocation.Offset, meshAlloc.VertexGlobalAllocation.Size);
				frameAllocator.AddAllocation(positionData.data(),
					&m_PositionBuffer, sizeof(positionData[0]) * (meshAlloc.VertexGlobalAllocation.Offset / sizeof(vertexData[0])),
					sizeof(positionData[0]) * (meshAlloc.VertexGlobalAllocation.Size / sizeof(vertexData[0])));

				m_MeshBufferMap[meshAlloc.MeshletGlobalAllocation.Offset] = meshAlloc;

				return { meshAlloc.MeshletGlobalAllocation.Offset, meshAlloc.VertexGlobalAllocation.Offset };
			}

			uint32_t UpdateRenderState(FrameStagingAllocator& frameAllocator, uint32_t materialIndex, uint32_t albedoIndex, uint32_t normalMapIndex)
			{
				uint32_t index = materialIndex;
				if (index == std::numeric_limits<decltype(materialIndex)>::max())
				{
					index = m_MaterialDataBuffer.Allocate();
				}

				MaterialData data;
				data.AlbedoDescriptorIndex = albedoIndex;
				data.NormalDescriptorIndex = normalMapIndex;
				frameAllocator.AddAllocation(&data, &m_MaterialDataBuffer.GetBuffer(), index, sizeof(data));

				return index;
			}

			uint32_t UpdateRenderState(ID3D12DeviceX* device, RenderAPI::CommandList& cmdList,
				RenderAPI::ResourceRecycler& recycler, RenderAPI::DescriptorHeap& descriptorHeap,
				const std::vector<uint8_t>& texelData, uint32_t width, uint32_t height, DXGI_FORMAT format
				)
			{
				auto descriptor = descriptorHeap.CreateDescriptor();
				uint32_t index = descriptor.GetHeapIndex();

				m_TextureMap[index] = RenderAPI::Texture2D(device, RenderAPI::Texture2D::Desc(width, height, format, D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS), &recycler, {});
				m_TextureDescriptorMap[index] = RenderAPI::ShaderResourceView(device, descriptorHeap, m_TextureMap[index]);

				const uint64_t stagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(m_TextureMap[index].GetResource(), 0, 1));
				RenderAPI::Buffer stagingBuffer(device, RenderAPI::Buffer::Desc(stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD), &recycler);

				D3D12_SUBRESOURCE_DATA subresourceData{};
				subresourceData.pData = texelData.data();
				subresourceData.RowPitch = /*roundUp(*/width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/; // todo Rounding needed?
				subresourceData.SlicePitch = subresourceData.RowPitch * height;

				UpdateSubresources(
					cmdList.Get(),
					m_TextureMap[index].GetResource(),
					stagingBuffer.GetResource(),
					0, 0, 1, &subresourceData);

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

			RenderAPI::Buffer m_PositionBuffer;
			RenderAPI::Buffer m_VertexBuffer;
			aZero::FreelistAllocator m_VertexFreelist;

			std::unordered_map<uint32_t, MeshAllocation> m_MeshBufferMap; // MeshletGlobalAllocation.Offset is the key

			// --------------------------------------------------------------------------------------------------------------
			//			MATERIAL
			// --------------------------------------------------------------------------------------------------------------
			RenderAPI::IndexedBuffer<MaterialData> m_MaterialDataBuffer;
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