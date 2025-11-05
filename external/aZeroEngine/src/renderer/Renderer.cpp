#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "assets/AssetManager.hpp"
#include "RenderSurface.hpp"
#include "pipeline/PipelineManager.hpp"
#include "pipeline/ScenePass.hpp"

namespace aZero
{
	namespace Rendering
	{
		Renderer::Renderer(ID3D12Device* device, uint32_t bufferCount, Pipeline::PipelineManager& diPipelineManager)
		{
			m_diDevice = device;
			m_BufferCount = bufferCount;
			m_diPipelineManager = &diPipelineManager;

			this->InitCommandRecording();
			this->InitDescriptorHeaps();
			this->InitFrameResources();
			this->InitSamplers();
			this->InitRenderPasses();
		}

		void Renderer::BeginFrame()
		{
			m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_BufferCount);
		}

		void Renderer::CopyTextureToTexture(ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture)
		{
			auto CmdContextHandle = m_CommandContextAllocator.GetContext();
			if (!CmdContextHandle.has_value())
			{
				throw std::runtime_error("No more command contexts");
			}

			D3D12::CommandContext& CmdContext = *CmdContextHandle->m_Context;

			auto x = DstTexture->GetDesc();
			auto y = SrcTexture->GetDesc();
			ID3D12GraphicsCommandList* CmdList = CmdContext.GetCommandList();

			std::vector<D3D12::ResourceTransitionBundles> PreCopyBarriers;
			PreCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, DstTexture });
			PreCopyBarriers.push_back({ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE, SrcTexture });
			D3D12::TransitionResources(CmdList, PreCopyBarriers);

			CmdList->CopyResource(DstTexture, SrcTexture);

			std::vector<D3D12::ResourceTransitionBundles> PostCopyBarriers;
			PostCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, DstTexture });
			PostCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, SrcTexture });
			D3D12::TransitionResources(CmdList, PostCopyBarriers);

			m_GraphicsQueue.ExecuteContext(CmdContext);
		}

		void Renderer::EndFrame()
		{
			if (m_FrameCount % m_BufferCount == 0)
			{
				m_GraphicsQueue.FlushCommands();
				m_ResourceRecycler.ReleaseResources();
				m_CommandContextAllocator.FreeCommandBuffers();

				this->UploadStagedAssets();

				m_AssetStagingAllocator.Reset();
			}

			m_FrameCount++;
		}

		void Renderer::Render(Scene::Scene& Scene, Rendering::RenderSurface& RenderSurface, bool ClearRenderSurface, Rendering::RenderSurface& DepthSurface, bool ClearDepthSurface)
		{
			/* BASIC STRUCTURE
			for each active camera:
				for each visual entity in the scene:
					perform culling
					add to batch if mesh type

				for each light:
					UpdateRenderState data to the gpu

				for each render pass:
					for each batch:
						bind object render pipeline & buffers
						UpdateRenderState data to the gpu
						perform draw call to render surfaces
			*/

			this->UploadStagedAssets();

			auto prepRenderCommandContext = m_CommandContextAllocator.GetContext();
			if (!prepRenderCommandContext.has_value())
				throw;

			ID3D12GraphicsCommandList* cmdList = prepRenderCommandContext.value().m_Context->GetCommandList();

			if (ClearRenderSurface)
			{
				RenderSurface.RecordClear(cmdList);
			}

			if (ClearDepthSurface)
			{
				DepthSurface.RecordClear(cmdList);
			}
			
			m_GraphicsQueue.ExecuteContext(*prepRenderCommandContext.value().m_Context);

			auto& rtv = RenderSurface.GetView<D3D12::RenderTargetView>();
			auto& dsv = DepthSurface.GetView<D3D12::DepthStencilView>();
			m_DefaultRenderPass.BindRenderTarget(0, &rtv);
			m_DefaultRenderPass.BindDepthStencil(&dsv);

			// TODO: Try to reuse the allocated memory for the batches in-between frames
			// TODO: Remove "frameBufferSize" once the buffers auto-expand once they are full
			const uint64_t frameBufferSize = 10000;
			LinearAllocator frameBatchAllocator(m_StaticMeshFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);

			std::unordered_map<uint32_t, std::unordered_map<uint32_t, Pipeline::ScenePass::StaticMeshBatchDrawData>> batches;

			LinearAllocator framePointLightAllocator(m_PointLightFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);
			std::vector<Scene::SceneProxy::PointLight> pointLights;
			
			LinearAllocator frameSpotLightAllocator(m_SpotLightFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);
			std::vector<Scene::SceneProxy::SpotLight> spotLights;

			LinearAllocator frameDirectionalLightAllocator(m_DirectionalLightFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);
			std::vector<Scene::SceneProxy::DirectionalLight> directionalLights;

			const Scene::SceneProxy& sceneProxy = Scene.GetProxy();
			for (const Scene::SceneProxy::Camera& camera : sceneProxy.m_Cameras.GetData())
			{
				for (const Scene::SceneProxy::StaticMesh& staticMeshEntity : sceneProxy.m_StaticMeshes.GetData())
				{
					if (true/* TODO: Perform frustrum culling */)
					{
						Pipeline::ScenePass::StaticMeshInstanceData instanceData;
						instanceData.Transform = staticMeshEntity.m_Transform;
						auto batch = batches[staticMeshEntity.m_MeshIndex][staticMeshEntity.m_MaterialIndex];
						batch.NumVertices = staticMeshEntity.m_NumVertices;
						batch.InstanceData.emplace_back();
					}
				}

				for (const Scene::SceneProxy::PointLight& pointLight : sceneProxy.m_PointLights.GetData())
				{
					if (true/* TODO: Perform frustrum culling */)
					{
						pointLights.emplace_back(pointLight);
					}
				}

				for (const Scene::SceneProxy::SpotLight& spotLight : sceneProxy.m_SpotLights.GetData())
				{
					if (true/* TODO: Perform frustrum culling */)
					{
						spotLights.emplace_back(spotLight);
					}
				}

				LinearAllocator<>::Allocation pointLightAllocation = framePointLightAllocator.Append((void*)pointLights.data(), pointLights.size() * sizeof(decltype(pointLights)::value_type));
				LinearAllocator<>::Allocation spotLightAllocation = frameSpotLightAllocator.Append((void*)spotLights.data(), spotLights.size() * sizeof(decltype(spotLights)::value_type));
				
				// Fetching all directional lights from the scene since nothing gets culled from it
				LinearAllocator<>::Allocation directionalLightAllocation = frameDirectionalLightAllocator.Append((void*)sceneProxy.m_DirectionalLights.GetData().data(), sceneProxy.m_DirectionalLights.GetData().size() * sizeof(decltype(sceneProxy.m_DirectionalLights.GetData())::value_type));

				{
					struct VertexPerPassData
					{
						DXM::Matrix ViewProjectionMatrix;
					};

					struct PixelPerPassData
					{
						uint32_t samplerIndex;
					};

					VertexPerPassData vertexShaderPassData;
					vertexShaderPassData.ViewProjectionMatrix = camera.m_ViewProjectionMatrix;

					PixelPerPassData pixelShaderPassData;
					pixelShaderPassData.samplerIndex = m_AnisotropicSampler.GetHeapIndex();

					m_DefaultRenderPass.BindBuffer("PositionBuffer", Pipeline::SHADER_TYPE::VS, &m_MeshBuffers.m_PositionBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("UVBuffer", Pipeline::SHADER_TYPE::VS, &m_MeshBuffers.m_UVBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("NormalBuffer", Pipeline::SHADER_TYPE::VS, &m_MeshBuffers.m_NormalBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("TangentBuffer", Pipeline::SHADER_TYPE::VS, &m_MeshBuffers.m_TangentBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("IndexBuffer", Pipeline::SHADER_TYPE::VS, &m_MeshBuffers.m_TangentBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("MeshEntryBuffer", Pipeline::SHADER_TYPE::VS, &m_MeshBuffers.m_MeshEntryBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("InstanceBuffer", Pipeline::SHADER_TYPE::VS, &m_StaticMeshFrameBuffers.at(m_FrameIndex));

					m_DefaultRenderPass.BindBuffer("PointLightBuffer", Pipeline::SHADER_TYPE::PS, &m_PointLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("SpotLightBuffer", Pipeline::SHADER_TYPE::PS, &m_SpotLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("DirectionalLightBuffer", Pipeline::SHADER_TYPE::PS, &m_DirectionalLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("MaterialBuffer", Pipeline::SHADER_TYPE::PS, &m_MaterialBuffer.GetBuffer());
					m_DefaultRenderPass.BindConstant("PixelShaderConstantsBuffer", Pipeline::SHADER_TYPE::PS, &pixelShaderPassData, sizeof(pixelShaderPassData));

					for (const auto& [meshIndex, batchArrayMap] : batches)
					{
						for (const auto& [materialIndex, batchArray] : batchArrayMap)
						{
							// TODO: Handle auto-resizing of the buffers if the allocator memory pool gets full
							LinearAllocator<>::Allocation batchAllocation = frameBatchAllocator.Append((void*)batchArray.InstanceData.data(), batchArray.InstanceData.size() * sizeof(decltype(batchArray.InstanceData)::value_type));
							MeshHandle meshHandle;
							meshHandle.NumVertices = batchArray.NumVertices;
							meshHandle.StartIndex = meshIndex;

							MaterialHandle materialHandle;
							materialHandle.Index = materialIndex;

							struct VertexPerDrawData
							{
								uint32_t meshStartOffset;
								uint32_t batchStartOffset;
							};

							struct PixelPerDrawData
							{
								uint32_t materialIndex;
								uint32_t numDirectionalLights;
								uint32_t numPointLights;
								uint32_t numSpotLights;
							};

							VertexPerDrawData vertexPerDrawData;
							vertexPerDrawData.meshStartOffset = meshHandle.StartIndex;
							vertexPerDrawData.batchStartOffset = batchAllocation.Offset;

							PixelPerDrawData pixelPerDrawData;
							pixelPerDrawData.materialIndex = materialHandle.Index;
							pixelPerDrawData.numPointLights = pointLights.size() * sizeof(decltype(pointLights)::value_type);
							pixelPerDrawData.numSpotLights = spotLights.size() * sizeof(decltype(spotLights)::value_type);
							pixelPerDrawData.numDirectionalLights = sceneProxy.m_DirectionalLights.GetData().size() * sizeof(decltype(sceneProxy.m_DirectionalLights.GetData())::value_type);

							auto batchCommandContext = m_CommandContextAllocator.GetContext();
							if (!batchCommandContext.has_value())
								throw;

							auto batchCommandList = batchCommandContext.value().m_Context->GetCommandList();

							ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap.GetDescriptorHeap(), m_SamplerHeap.GetDescriptorHeap() };
							batchCommandList->SetDescriptorHeaps(2, Heaps);

							if (!m_DefaultRenderPass.BeginPass(batchCommandList))
								throw;

							batchCommandList->RSSetViewports(1, &camera.m_Viewport);
							batchCommandList->RSSetScissorRects(1, &camera.m_ScizzorRect);

							m_DefaultRenderPass.m_Pass.SetShaderResource(batchCommandList, "VertexPerPassData", (void*)&vertexShaderPassData, sizeof(vertexShaderPassData), Pipeline::SHADER_TYPE::VS);
							m_DefaultRenderPass.m_Pass.SetShaderResource(batchCommandList, "PerBatchConstantsBuffer", (void*)&vertexPerDrawData, sizeof(vertexPerDrawData), Pipeline::SHADER_TYPE::VS);

							uint32_t numVertices = meshHandle.NumVertices;
							batchCommandList->DrawInstanced(numVertices, batchArray.InstanceData.size(), 0, 0);

							m_GraphicsQueue.ExecuteContext(*batchCommandContext.value().m_Context);
						}
					}
				}
			}
		}

		void Renderer::InitCommandRecording()
		{
			m_GraphicsQueue.Init(m_diDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_CommandContextAllocator.Init(m_diDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, 10);
		}

		void Renderer::InitDescriptorHeaps()
		{
			m_ResourceHeap.Init(m_diDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);
			m_SamplerHeap.Init(m_diDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeap.Init(m_diDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeap.Init(m_diDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);
		}

		void Renderer::InitFrameResources()
		{
			// TODO: Remove once the buffers auto-expand once they are full
			const uint64_t frameBufferSize = 10000;
			for (int i = 0; i < m_BufferCount; i++)
			{
				m_StaticMeshFrameBuffers.emplace_back(D3D12::GPUBuffer(m_diDevice, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));
				
				m_PointLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_diDevice, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));

				m_SpotLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_diDevice, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));

				m_DirectionalLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_diDevice, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));
			}

			// TODO: Change so these use 64bit ints
			m_AssetStagingAllocator.Init(m_diDevice, sizeof(int32_t) * 100000, m_ResourceRecycler);
			m_MeshBuffers.m_PositionBuffer.Init(m_diDevice, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_UVBuffer.Init(m_diDevice, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_NormalBuffer.Init(m_diDevice, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_TangentBuffer.Init(m_diDevice, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_IndexBuffer.Init(m_diDevice, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_MeshEntryBuffer.Init(m_diDevice, sizeof(int32_t) * 10000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
		
			m_MaterialBuffer.Init(m_diDevice, sizeof(MaterialHandle) * 1000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
		}

		void Renderer::InitSamplers()
		{
			m_AnisotropicSampler = m_SamplerHeap.GetDescriptor();
			D3D12_SAMPLER_DESC SamplerDesc;
			SamplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
			SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			SamplerDesc.MipLODBias = 0.f;
			SamplerDesc.MaxAnisotropy = 8;
			SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
			SamplerDesc.MinLOD = 0;
			SamplerDesc.MaxLOD = 6;
			m_diDevice->CreateSampler(&SamplerDesc, m_AnisotropicSampler.GetCPUHandle());
		}

		void Renderer::InitRenderPasses()
		{
			m_BasePassVS.CompileFromFile(m_diPipelineManager->GetCompiler(), m_diPipelineManager->GetShaderFolderPath() + "BasePass.vs.hlsl");
			m_BasePassPS.CompileFromFile(m_diPipelineManager->GetCompiler(), m_diPipelineManager->GetShaderFolderPath() + "BasePass.ps.hlsl");

			Pipeline::RenderPass Pass;
			Pass.Init(m_diDevice, m_BasePassVS, m_BasePassPS, {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB}, DXGI_FORMAT_D24_UNORM_S8_UINT);
			m_DefaultRenderPass = RenderGraphPass(std::move(Pass));
		}

		void Renderer::HotreloadRenderPasses()
		{
			try
			{
				this->FlushGraphicsQueue();
				this->InitRenderPasses();
			}
			catch(const std::exception& e)
			{
				DEBUG_PRINT("Failed to hotreload the render pass");
			}
		}

		void Renderer::UploadStagedAssets()
		{
			auto cmdContext = m_CommandContextAllocator.GetContext();
			if (!cmdContext.has_value())
				throw;

			ID3D12GraphicsCommandList* cmdList = cmdContext.value().m_Context->GetCommandList();

			m_AssetStagingAllocator.RecordAllocations(cmdList);

			m_GraphicsQueue.ExecuteContext(*cmdContext.value().m_Context);
		}

		void Renderer::AllocateFreelistMesh(Asset::Mesh& mesh, ID3D12GraphicsCommandList* cmdList)
		{
			int allocSize = mesh.m_VertexData.Positions.size() * sizeof(mesh.m_VertexData.Positions.at(0)); // TODO: Indices might be larger...
			DS::FreelistAllocator::AllocationHandle allocHandle;

			m_MeshBuffers.m_PositionBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_PositionBuffer.Write(cmdList, m_AssetStagingAllocator, allocHandle, mesh.m_VertexData.Positions.data());
			m_MeshBuffers.m_PositionAllocMap[mesh.GetAssetID()] = std::move(allocHandle);

			m_MeshBuffers.m_NormalBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_NormalBuffer.Write(cmdList, m_AssetStagingAllocator, allocHandle, mesh.m_VertexData.Normals.data());
			m_MeshBuffers.m_NormalAllocMap[mesh.GetAssetID()] = std::move(allocHandle);

			m_MeshBuffers.m_UVBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_UVBuffer.Write(cmdList, m_AssetStagingAllocator, allocHandle, mesh.m_VertexData.UVs.data());
			m_MeshBuffers.m_UVAllocMap[mesh.GetAssetID()] = std::move(allocHandle);

			m_MeshBuffers.m_TangentBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_TangentBuffer.Write(cmdList, m_AssetStagingAllocator, allocHandle, mesh.m_VertexData.Tangents.data());
			m_MeshBuffers.m_TangentAllocMap[mesh.GetAssetID()] = std::move(allocHandle);

			m_MeshBuffers.m_IndexBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_IndexBuffer.Write(cmdList, m_AssetStagingAllocator, allocHandle, mesh.m_VertexData.Indices.data());
			m_MeshBuffers.m_IndexAllocMap[mesh.GetAssetID()] = std::move(allocHandle);
		}

		void Renderer::UpdateRenderState(Asset::AssetHandle<Asset::Mesh>& mesh)
		{
			if (!mesh.IsValid())
			{
				DEBUG_PRINT("Invalid mesh handle.");
			}

			auto& meshAsset = *mesh.GetAsset();

			if (meshAsset.m_VertexData.Positions.size() == 0)
			{
				return;
			}

			// TODO: implement resize of FreelistBuffer with this cmdlist, otherwise it isnt needed
			auto cmdList = m_CommandContextAllocator.GetContext()->m_Context->GetCommandList();

			if (meshAsset.GetRenderID() == std::numeric_limits<Asset::RenderID>::max())
			{
				this->AllocateFreelistMesh(meshAsset, cmdList);

				DS::FreelistAllocator::AllocationHandle allocHandle;
				MeshHandle meshHandle;
				meshHandle.StartIndex = m_MeshBuffers.m_IndexAllocMap[meshAsset.GetAssetID()].GetStartOffset() / sizeof(meshAsset.m_VertexData.Positions.at(0));
				meshHandle.NumVertices = meshAsset.m_VertexData.Indices.size();
				m_MeshBuffers.m_MeshEntryBuffer.Allocate(allocHandle, sizeof(MeshHandle));
				m_MeshBuffers.m_MeshEntryBuffer.Write(cmdList, m_AssetStagingAllocator, allocHandle, &meshHandle);
				m_MeshBuffers.m_MeshEntryAllocMap[meshAsset.GetAssetID()] = std::move(allocHandle);
				meshAsset.m_RenderID = allocHandle.GetStartOffset() / sizeof(MeshHandle);
			}
			else
			{
				const Asset::AssetID id = meshAsset.GetAssetID();
				m_MeshBuffers.m_PositionAllocMap.erase(id);
				m_MeshBuffers.m_UVAllocMap.erase(id);
				m_MeshBuffers.m_NormalAllocMap.erase(id);
				m_MeshBuffers.m_TangentAllocMap.erase(id);
				m_MeshBuffers.m_IndexAllocMap.erase(id);

				this->AllocateFreelistMesh(meshAsset, cmdList);
				MeshHandle meshHandle;
				meshHandle.StartIndex = m_MeshBuffers.m_IndexAllocMap[id].GetStartOffset() / sizeof(meshAsset.m_VertexData.Positions.at(0));
				meshHandle.NumVertices = meshAsset.m_VertexData.Indices.size();
				m_MeshBuffers.m_MeshEntryBuffer.Write(cmdList, m_AssetStagingAllocator, m_MeshBuffers.m_MeshEntryAllocMap[id], &meshHandle);
			}
		}

		void Renderer::UpdateRenderState(Asset::AssetHandle<Asset::Material>& material)
		{
			if (!material.IsValid())
			{
				DEBUG_PRINT("Invalid material handle.");
			}

			Asset::Material* materialAsset = material.GetAsset();
			Asset::Texture* albedoTexture = materialAsset->m_Data.AlbedoTexture.GetAsset();
			Asset::Texture* normalTexture = materialAsset->m_Data.NormalMap.GetAsset();

			DS::FreelistAllocator::AllocationHandle allocHandle;

			struct MaterialData
			{
				int32_t AlbedoIndex;
				int32_t NormalIndex;
			};
			MaterialData materialData;

			if (albedoTexture == nullptr || (albedoTexture && albedoTexture->GetRenderID() == std::numeric_limits<Asset::RenderID>::max()))
			{
				materialData.AlbedoIndex = -1;
			}
			else
			{
				materialData.AlbedoIndex = albedoTexture->GetRenderID();
			}

			if (normalTexture == nullptr || (normalTexture && normalTexture->GetRenderID() == std::numeric_limits<Asset::RenderID>::max()))
			{
				materialData.NormalIndex = -1;
			}
			else
			{
				materialData.NormalIndex = normalTexture->GetRenderID();
			}

			if (materialAsset->GetRenderID() == std::numeric_limits<Asset::RenderID>::max())
			{
				m_MaterialBuffer.Allocate(allocHandle, sizeof(materialData));
				materialAsset->m_RenderID = allocHandle.GetStartOffset() / sizeof(materialData);
				m_MaterialAllocMap[materialAsset->GetAssetID()] = std::move(allocHandle);
			}
				
			m_MaterialBuffer.Write(nullptr, m_AssetStagingAllocator, m_MaterialAllocMap[materialAsset->GetAssetID()], &materialData);
		}

		void Renderer::UpdateRenderState(Asset::AssetHandle<Asset::Texture>& texture)
		{
			if (!texture.IsValid())
			{
				DEBUG_PRINT("Invalid texture handle.");
			}

			Asset::Texture* textureAsset = texture.GetAsset();

			if (textureAsset->m_Data.TexelData.size() 
				== textureAsset->m_Data.Height * textureAsset->m_Data.Width * textureAsset->m_Data.NumChannels)
			{
				DEBUG_PRINT("Texel data size doesn't match the Texture dimensions and channel count.");
			}

			TextureRenderAsset assetData;
			assetData.m_Resource = D3D12::GPUTexture(m_diDevice,
				DXM::Vector3(textureAsset->m_Data.Width, textureAsset->m_Data.Height, 1),
				DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
				m_ResourceRecycler,
				1);

			// TODO: Be sure the old texture isnt accessed when/after the same descriptor index is reallocated
				// We need to use the same descriptor index since we need to keep the renderID valid
			assetData.m_Srv.Init(m_diDevice, m_ResourceHeap.GetDescriptor(), assetData.m_Resource, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

			if (textureAsset->GetRenderID() == std::numeric_limits<Asset::RenderID>::max())
			{
				textureAsset->m_RenderID = assetData.m_Srv.GetDescriptorIndex();
			}

			// UpdateRenderState texel data
			const uint64_t StagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(assetData.m_Resource.GetResource(), 0, 1));
			D3D12::GPUBuffer StagingBuffer;
			StagingBuffer.Init(m_diDevice, D3D12_HEAP_TYPE_UPLOAD, StagingBufferSize, m_ResourceRecycler);

			std::vector<D3D12::ResourceTransitionBundles> Bundle;
			Bundle.push_back({});
			Bundle.at(0).ResourcePtr = assetData.m_Resource.GetResource();
			Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COMMON;
			Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

			auto CmdContext = m_CommandContextAllocator.GetContext();

			ID3D12GraphicsCommandList* CmdList = CmdContext->m_Context->GetCommandList();
			D3D12::TransitionResources(CmdList, Bundle);

			D3D12_SUBRESOURCE_DATA SubresourceData{};
			SubresourceData.pData = textureAsset->m_Data.TexelData.data();
			SubresourceData.RowPitch = /*roundUp(*/textureAsset->m_Data.Width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
			SubresourceData.SlicePitch = SubresourceData.RowPitch * textureAsset->m_Data.Height;

			UpdateSubresources(
				CmdList,
				assetData.m_Resource.GetResource(),
				StagingBuffer.GetResource(),
				0, 0, 1, &SubresourceData);

			Bundle.at(0).ResourcePtr = assetData.m_Resource.GetResource();
			Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			D3D12::TransitionResources(CmdList, Bundle);

			m_GraphicsQueue.ExecuteContext(*CmdContext->m_Context);


			m_TextureAllocMap[textureAsset->GetAssetID()] = std::move(assetData);
		}

		void Renderer::RemoveRenderState(Asset::AssetHandle<Asset::Mesh>& mesh)
		{
			Asset::Mesh* meshAsset = mesh.GetAsset();
			if (meshAsset && meshAsset->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
			{
				Asset::AssetID id = meshAsset->GetAssetID();
				meshAsset->m_RenderID = std::numeric_limits<Asset::RenderID>::max();
				m_MeshBuffers.m_PositionAllocMap.erase(id);
				m_MeshBuffers.m_UVAllocMap.erase(id);
				m_MeshBuffers.m_NormalAllocMap.erase(id);
				m_MeshBuffers.m_TangentAllocMap.erase(id);
				m_MeshBuffers.m_IndexAllocMap.erase(id);
				m_MeshBuffers.m_MeshEntryAllocMap.erase(id);
			}
		}

		void Renderer::RemoveRenderState(Asset::AssetHandle<Asset::Material>& material)
		{
			Asset::Material* materialAsset = material.GetAsset();
			if (materialAsset && materialAsset->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
			{
				materialAsset->m_RenderID = std::numeric_limits<Asset::RenderID>::max();
				m_MaterialAllocMap.erase(materialAsset->GetAssetID());
			}
		}

		void Renderer::RemoveRenderState(Asset::AssetHandle<Asset::Texture>& texture)
		{
			Asset::Texture* textureAsset = texture.GetAsset();
			if (textureAsset->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
			{
				textureAsset->m_RenderID = std::numeric_limits<Asset::RenderID>::max();

				// TODO: Remove it after its last usage so we dont create a new view with the same descriptor index while its still accessed on the gpu
				m_TextureAllocMap.erase(textureAsset->GetAssetID());
			}
		}

		// TODO: Reimplement to fit the new renderer
		//void Renderer::RenderPrimitiveBatch(D3D12::CommandContext& CmdContext, const D3D12::RenderTargetView& ColorView, const PrimitiveBatch& Batch, const Scene::Scene::Camera& Camera)
		//{
		//	if (Batch.IsDrawable())
		//	{
		//		ID3D12GraphicsCommandList* CmdList = CmdContext.GetCommandList();

		//		m_BatchVertexBuffer.Write(CmdList, Batch.GetData(), Batch.GetNumBytes());

		//		this->GetFrameAllocator().GetCommandContext().StopRecording();
		//		m_GraphicsQueue.ExecuteContext(this->GetFrameAllocator().GetCommandContext());

		//		ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap.GetDescriptorHeap(), m_SamplerHeap.GetDescriptorHeap() };
		//		CmdList->SetDescriptorHeaps(2, Heaps);

		//		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles = { ColorView.GetDescriptorHandle() };

		//		Pipeline::RenderPass* Pass = nullptr;
		//		const PrimitiveBatch::RenderLayer RenderLayer = Batch.GetLayer();
		//		const D3D_PRIMITIVE_TOPOLOGY PrimitiveType = Batch.GetPrimitiveType();
		//		if (RenderLayer == PrimitiveBatch::RenderLayer::DEPTH)
		//		{
		//			if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_POINTLIST)
		//			{
		//				Pass = &m_BatchPassDepthP;
		//			}
		//			else if ((PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINELIST) || (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP))
		//			{
		//				Pass = &m_BatchPassDepthL;
		//			}
		//			else if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
		//			{
		//				Pass = &m_BatchPassDepthT;
		//			}
		//		}
		//		else if (RenderLayer == PrimitiveBatch::RenderLayer::NO_DEPTH)
		//		{
		//			if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_POINTLIST)
		//			{
		//				Pass = &m_BatchPassNoDepthP;
		//			}
		//			else if ((PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINELIST) || (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP))
		//			{
		//				Pass = &m_BatchPassNoDepthL;
		//			}
		//			else if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
		//			{
		//				Pass = &m_BatchPassNoDepthT;
		//			}
		//		}

		//		CmdList->SetPipelineState(Pass->GetPipelineState());
		//		CmdList->SetGraphicsRootSignature(Pass->GetRootSignature());

		//		CmdList->IASetPrimitiveTopology(PrimitiveType);

		//		CmdList->RSSetViewports(1, &Camera.Viewport);
		//		CmdList->RSSetScissorRects(1, &Camera.ScizzorRect);

		//		struct VertexShaderConstants
		//		{
		//			DXM::Matrix ViewProjectionMatrix;
		//		};

		//		VertexShaderConstants VSConstants;
		//		VSConstants.ViewProjectionMatrix = Camera.ViewMatrix * Camera.ProjMatrix;

		//		Pass->SetShaderResource(CmdList, "CameraDataBuffer", &VSConstants, sizeof(VertexShaderConstants), Pipeline::SHADER_TYPE::VS);

		//		D3D12_VERTEX_BUFFER_VIEW VBView;
		//		VBView.BufferLocation = m_BatchVertexBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress();
		//		VBView.SizeInBytes = Batch.GetNumBytes();
		//		VBView.StrideInBytes = sizeof(PrimitiveBatch::Point);
		//		CmdList->IASetVertexBuffers(0, 1, &VBView);

		//		CmdList->DrawInstanced(static_cast<UINT>(Batch.GetNumPoints()), 1, 0, 0);

		//		CmdContext.StopRecording();
		//		m_GraphicsQueue.ExecuteContext(CmdContext);
		//	}
		//}
		
		// TODO: Reimplement to fit the new renderer
		//void Renderer::InitPrimitiveBatchPipeline()
		//{
		//	m_BatchVertexBuffer.Init(m_diDevice, D3D12_HEAP_TYPE_UPLOAD, 2, m_ResourceRecycler);

		//	D3D12::Shader PassVS;
		//	PassVS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "PrimitiveBatch.vs.hlsl");

		//	D3D12::Shader PassPS;
		//	PassPS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "PrimitiveBatch.ps.hlsl");

		//	m_BatchPassDepthP.Init(m_diDevice, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		//	m_BatchPassNoDepthP.Init(m_diDevice, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		//	m_BatchPassDepthL.Init(m_diDevice, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		//	m_BatchPassNoDepthL.Init(m_diDevice, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		//	m_BatchPassDepthT.Init(m_diDevice, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//	m_BatchPassNoDepthT.Init(m_diDevice, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//}
	}
}