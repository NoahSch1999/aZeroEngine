#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "RenderSurface.hpp"

namespace aZero
{
	namespace Rendering
	{
		Renderer::Renderer(ID3D12DeviceX* device, uint32_t bufferCount, IDxcCompilerX& compiler)
			:m_Compiler(compiler)
		{
			m_diDevice = device;
			m_BufferCount = bufferCount;

			this->Init(device, bufferCount);
		}

		void Renderer::Init(ID3D12DeviceX* device, uint32_t numFramesInFlight)
		{
			m_DirectCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_CopyCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY);
			m_ComputeCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

			m_ResourceHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, true);
			m_SamplerHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);
			
			// TODO: DEFINE A MOVE-CONSTRUCTOR FOR FRAMECONTEXT
			// OTHERWISE THIS WILL CRASH IF WE DONT RESERVE
			m_FrameContexts.reserve(numFramesInFlight);
			for (int32_t i = 0; i < numFramesInFlight; i++)
			{
				m_FrameContexts.emplace_back(device, m_ResourceHeapNew, m_NewResourceRecycler);
			}

			m_SamplerManager = SamplerManager(device, m_SamplerHeapNew);

			m_MeshEntryBuffer = RenderAPI::IndexedBuffer<MeshEntry>(device, 1000, &m_NewResourceRecycler);
			m_NewMaterialBuffer = RenderAPI::IndexedBuffer<MaterialData>(device, 1000, &m_NewResourceRecycler);
		}

		bool Renderer::AdvanceFrameIfReady()
		{
			const uint32_t newPotentialFrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());

			if (!m_FrameContexts.at(newPotentialFrameIndex).IsReady(m_DirectCommandQueue))
			{
				return false;
			}

			this->GetCurrentContext().Begin();

			return true;
		}

		bool Renderer::BeginFrame()
		{
			const bool hasNewFrameStarted = AdvanceFrameIfReady();
			if (hasNewFrameStarted)
			{
				m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());
				m_FrameCount++;
			}

			return hasNewFrameStarted;
		}

		void Renderer::Render(const Scene::Scene& scene, std::optional<Rendering::RenderTarget*> renderTarget, std::optional<Rendering::DepthTarget*> depthTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			// Perform uploads for all updated/new assets and other stagings
			frameContext.RecordFrameAllocations(frameContext.m_DirectCmdList);
			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);

			// todo Do rendering

			//
		}

		void Renderer::FlushGPU()
		{
			m_DirectCommandQueue.Flush();

			// todo When we're also using other types of queues we need to add them here and do some other stuff
		}

		void Renderer::CopyTextureToTexture(RenderAPI::Texture2D& dstTexture, RenderAPI::Texture2D& srcTexture)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			ID3D12Resource* dstResource = dstTexture.GetResource();
			ID3D12Resource* srcResource = srcTexture.GetResource();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, dstResource });
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, preCopyBarriers);

			frameContext.m_DirectCmdList->CopyResource(dstTexture.GetResource(), srcTexture.GetResource());

			std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, dstResource });
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, postCopyBarriers);

			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);
		}

		void Renderer::UpdateRenderState(Asset::Mesh* mesh)
		{
			if (!mesh)
			{
				DEBUG_PRINT("Invalid mesh handle.");
				throw;
			}

			const Asset::Mesh::Data& vertexData = mesh->GetVertexData();
			if (vertexData.Positions.empty())
			{
				return;
			}

			FrameContext& frameContext = this->GetCurrentContext();

			// todo Handle re-create of the descriptors
			if (mesh->GetRenderID() == std::numeric_limits<Asset::RenderID>::max())
			{
				m_GPUMeshes.emplace(mesh->GetRenderID(), RenderAPI::VertexBuffer(m_diDevice, frameContext.m_DirectCmdList, m_NewResourceRecycler, vertexData, m_ResourceHeapNew));

				const auto& gpuMesh = m_GPUMeshes.at(mesh->GetRenderID());
				MeshEntry meshHandle;
				meshHandle.PositionsIndex = gpuMesh.m_Positions.m_Descriptor.GetHeapIndex();
				meshHandle.UVsIndex = gpuMesh.m_UVs.m_Descriptor.GetHeapIndex();
				meshHandle.NormalsIndex = gpuMesh.m_Normals.m_Descriptor.GetHeapIndex();
				meshHandle.TangentsIndex = gpuMesh.m_Tangents.m_Descriptor.GetHeapIndex();
				meshHandle.IndicesIndex = gpuMesh.m_Indices.m_Descriptor.GetHeapIndex();
				meshHandle.NumIndices = static_cast<uint32_t>(vertexData.Indices.size());

				const uint32_t index = m_MeshEntryBuffer.Allocate();
				mesh->m_RenderID = index;

				frameContext.AddAllocation(meshHandle, m_MeshEntryBuffer.GetBuffer(), m_MeshEntryBuffer.GetByteOffset(index));
				m_NewMeshEntryAllocMap[mesh->GetRenderID()] = index;
			}
			else
			{
				const Asset::RenderID id = mesh->GetRenderID();
				m_GPUMeshes.erase(id);

				m_GPUMeshes.emplace(mesh->GetRenderID(), RenderAPI::VertexBuffer(m_diDevice, frameContext.m_DirectCmdList, m_NewResourceRecycler, vertexData, m_ResourceHeapNew));

				const auto& gpuMesh = m_GPUMeshes.at(mesh->GetRenderID());
				MeshEntry meshHandle;
				meshHandle.PositionsIndex = gpuMesh.m_Positions.m_Descriptor.GetHeapIndex();
				meshHandle.UVsIndex = gpuMesh.m_UVs.m_Descriptor.GetHeapIndex();
				meshHandle.NormalsIndex = gpuMesh.m_Normals.m_Descriptor.GetHeapIndex();
				meshHandle.TangentsIndex = gpuMesh.m_Tangents.m_Descriptor.GetHeapIndex();
				meshHandle.IndicesIndex = gpuMesh.m_Indices.m_Descriptor.GetHeapIndex();
				meshHandle.NumIndices = static_cast<uint32_t>(vertexData.Indices.size());

				frameContext.AddAllocation(meshHandle, m_MeshEntryBuffer.GetBuffer(), m_MeshEntryBuffer.GetByteOffset(m_NewMeshEntryAllocMap[id]));
			}
		}

		void Renderer::UpdateRenderState(Asset::Material* material)
		{
			if (!material)
			{
				DEBUG_PRINT("Invalid material handle.");
				throw;
			}

			const Asset::Texture* albedoTexture = material->m_Data.AlbedoTexture;
			const Asset::Texture* normalTexture = material->m_Data.NormalMap;
			
			MaterialData materialData;

			if (albedoTexture == nullptr || (albedoTexture && albedoTexture->GetRenderID() == Asset::InvalidRenderID))
			{
				materialData.AlbedoIndex = -1;
			}
			else
			{
				materialData.AlbedoIndex = albedoTexture->GetRenderID();
			}

			if (normalTexture == nullptr || (normalTexture && normalTexture->GetRenderID() == Asset::InvalidRenderID))
			{
				materialData.NormalIndex = -1;
			}
			else
			{
				materialData.NormalIndex = normalTexture->GetRenderID();
			}

			if (material->GetRenderID() == Asset::InvalidRenderID)
			{
				const uint32_t index = m_NewMaterialBuffer.Allocate();
				material->m_RenderID = index;

				m_NewMaterialAllocMap[material->m_RenderID] = index;
			}

			FrameContext& frameContext = this->GetCurrentContext();
			frameContext.AddAllocation(materialData, m_NewMaterialBuffer.GetBuffer(), m_NewMaterialBuffer.GetByteOffset(material->m_RenderID));
		}

		void Renderer::UpdateRenderState(Asset::Texture* texture)
		{
			if (!texture)
			{
				DEBUG_PRINT("Invalid texture handle.");
				throw;
			}

			if (texture->m_Data.TexelData.size()
				!= texture->m_Data.Height * texture->m_Data.Width * texture->m_Data.NumChannels)
			{
				// todo How to handle textures where the format doesnt match the channel count
				// todo Fix why this is triggered
				// Maybe use dxtk texture loading functions?
				DEBUG_PRINT("Texel data size doesn't match the Texture dimensions and channel count.");
				//return;
			}

			FrameContext& frameContext = this->GetCurrentContext();

			GPUTexture2D assetData;
			RenderAPI::Texture2D::Desc desc;
			desc.Format = texture->m_Data.Format;
			desc.Width = texture->m_Data.Width;
			desc.Height = texture->m_Data.Height;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
			assetData.m_Texture = RenderAPI::Texture2D(m_diDevice, desc, &m_NewResourceRecycler);

			// todo Make so that the old texture isnt accessed when/after the same descriptor index is reallocated
				// We need to use the same descriptor index since we need to keep the renderID valid
			assetData.m_Srv = m_ResourceHeapNew.CreateDescriptor();

			D3D12_RESOURCE_DESC resourceDesc = assetData.m_Texture.GetResource()->GetDesc();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.Format = resourceDesc.Format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

			// !note Rework once multiple mip-levels are supported
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = static_cast<FLOAT>(0);

			m_diDevice->CreateShaderResourceView(assetData.m_Texture.GetResource(), &srvDesc, assetData.m_Srv.GetCpuHandle());

			if (texture->GetRenderID() == Asset::InvalidRenderID)
			{
				texture->m_RenderID = assetData.m_Srv.GetHeapIndex();
			}

			// UpdateRenderState texel data
			const uint64_t stagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(assetData.m_Texture.GetResource(), 0, 1));
			RenderAPI::Buffer stagingBuffer(m_diDevice, RenderAPI::Buffer::Desc(stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD), &m_NewResourceRecycler);

			const D3D12_RESOURCE_BARRIER& preUploadBarrier = assetData.m_Texture.CreateTransition(D3D12_RESOURCE_STATE_COPY_DEST);

			// todo Change to enhanced barrier
			frameContext.m_DirectCmdList->ResourceBarrier(1, &preUploadBarrier);

			D3D12_SUBRESOURCE_DATA subresourceData{};
			subresourceData.pData = texture->m_Data.TexelData.data();
			subresourceData.RowPitch = /*roundUp(*/texture->m_Data.Width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
			subresourceData.SlicePitch = subresourceData.RowPitch * texture->m_Data.Height;

			UpdateSubresources(
				frameContext.m_DirectCmdList.Get(),
				assetData.m_Texture.GetResource(),
				stagingBuffer.GetResource(),
				0, 0, 1, &subresourceData);

			// todo Change to enhanced barrier
			const D3D12_RESOURCE_BARRIER& postUploadBarrier = assetData.m_Texture.CreateTransition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			frameContext.m_DirectCmdList->ResourceBarrier(1, &postUploadBarrier);

			frameContext.SetLatestSignal(m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList));

			m_GPUTextures[texture->m_RenderID] = std::move(assetData);
		}

		void Renderer::RemoveRenderState(Asset::Mesh* mesh)
		{
			if (mesh && mesh->GetRenderID() != Asset::InvalidRenderID)
			{
				// todo Remove it after its last usage so we dont create a new view with the same descriptor index while its still accessed on the gpu

				const Asset::RenderID id = mesh->GetRenderID();

				// Remove the vertex data
				m_GPUMeshes.erase(id);

				// Remove the mesh entry
				const uint32_t meshEntryIndex = m_NewMeshEntryAllocMap.at(id);
				m_MeshEntryBuffer.Deallocate(meshEntryIndex);
				m_NewMeshEntryAllocMap.erase(id);

				// Invalidate the assets render-resource reference id
				mesh->m_RenderID = Asset::InvalidRenderID;
			}
		}

		void Renderer::RemoveRenderState(Asset::Material* material)
		{
			if (material && material->GetRenderID() != Asset::InvalidRenderID)
			{

				const Asset::RenderID id = material->GetRenderID();

				// Remove the material
				const uint32_t meshEntryIndex = m_NewMaterialAllocMap.at(id);
				m_NewMaterialBuffer.Deallocate(meshEntryIndex);
				m_NewMaterialAllocMap.erase(id);

				// Invalidate the assets render-resource reference id
				material->m_RenderID = Asset::InvalidRenderID;
			}
		}

		void Renderer::RemoveRenderState(Asset::Texture* texture)
		{
			if (texture && texture->GetRenderID() != Asset::InvalidRenderID)
			{
				// todo Remove it after its last usage so we dont create a new view with the same descriptor index while its still accessed on the gpu
				m_GPUTextures.erase(texture->GetRenderID());

				// Invalidate the assets render-resource reference id
				texture->m_RenderID = Asset::InvalidRenderID;
			}
		}
	}
}