#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "assets/AssetManager.hpp"

namespace aZero
{
	namespace Rendering
	{
		void RendererNew::BeginFrame()
		{
			m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_BufferCount);
		}

		void RendererNew::CopyTextureToTexture(ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture)
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

		void RendererNew::EndFrame()
		{
			if (m_FrameCount % m_BufferCount == 0)
			{
				m_GraphicsQueue.FlushCommands();
				m_ResourceRecycler.ReleaseResources();
				m_CommandContextAllocator.FreeCommandBuffers();
			}

			m_FrameCount++;
		}

		void RendererNew::Render(Scene::SceneNew& Scene, const D3D12::RenderTargetView& RenderSurface, bool ClearRenderSurface, const D3D12::DepthStencilView& DepthSurface, bool ClearDepthSurface)
		{
			/* BASIC STRUCTURE
			for each active camera:
				for each visual entity in the scene:
					perform culling
					add to batch if mesh type

				for each light:
					upload data to the gpu

				for each render pass:
					for each batch:
						bind object render pipeline & buffers
						upload data to the gpu
						perform draw call to render surfaces
			*/

			auto prepRenderCommandContext = m_CommandContextAllocator.GetContext();
			if (!prepRenderCommandContext.has_value())
				throw;

			ID3D12GraphicsCommandList* prepRenderCommandList = prepRenderCommandContext.value().m_Context->GetCommandList();

			if (ClearRenderSurface)
			{
				prepRenderCommandList->ClearRenderTargetView(RenderSurface.GetDescriptorHandle(), RenderSurface.GetClearValue().Color, 0, NULL);
			}

			if (ClearDepthSurface)
			{
				prepRenderCommandList->ClearDepthStencilView(
					DepthSurface.GetDescriptorHandle(),
					D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
					DepthSurface.GetClearValue().DepthStencil.Depth, DepthSurface.GetClearValue().DepthStencil.Stencil,
					0, NULL);
			}
			
			m_GraphicsQueue.ExecuteContext(*prepRenderCommandContext.value().m_Context);

			struct StaticMeshInstanceData
			{
				DXM::Matrix Transform;
			};

			//// TODO: Try to reuse the allocated memory for the batches in-between frames
			//// TODO: Remove "frameBufferSize" once the buffers auto-expand once they are full
			const uint64_t frameBufferSize = 10000;
			LinearAllocator frameBatchAllocator(m_StaticMeshFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);

			struct StaticMeshBatchDrawData
			{
				std::vector<StaticMeshInstanceData> InstanceData;
				uint32_t NumVertices;
			};
			std::unordered_map<uint32_t, std::unordered_map<uint32_t, StaticMeshBatchDrawData>> batches;

			LinearAllocator framePointLightAllocator(m_PointLightFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);
			std::vector<Scene::SceneProxy::PointLight> pointLights;
			
			LinearAllocator frameSpotLightAllocator(m_SpotLightFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);
			std::vector<Scene::SceneProxy::SpotLight> spotLights;

			LinearAllocator frameDirectionalLightAllocator(m_DirectionalLightFrameBuffers.at(m_FrameIndex).GetCPUAccessibleMemory(), frameBufferSize);
			std::vector<Scene::SceneProxy::DirectionalLight> directionalLights;

			const Scene::SceneProxy& sceneProxy = Scene.m_Proxy;
			for (const Scene::SceneProxy::Camera& camera : sceneProxy.m_Cameras.GetData())
			{
				for (const Scene::SceneProxy::StaticMesh& staticMeshEntity : sceneProxy.m_StaticMeshes.GetData())
				{
					if (true/* TODO: Perform frustrum culling */)
					{
						StaticMeshInstanceData instanceData;
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

				// TODO: This should be done per pass, ex. depth pass -> geometry pass -> deffered light pass -> per camera post-process pass etc...
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

					// TODO: Remove this type of binding and use pure bindless
					m_DefaultRenderPass.BindBuffer("PositionBuffer", D3D12::SHADER_TYPE::VS, &m_MeshBuffers.m_PositionBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("UVBuffer", D3D12::SHADER_TYPE::VS, &m_MeshBuffers.m_UVBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("NormalBuffer", D3D12::SHADER_TYPE::VS, &m_MeshBuffers.m_NormalBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("TangentBuffer", D3D12::SHADER_TYPE::VS, &m_MeshBuffers.m_TangentBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("IndexBufferNew", D3D12::SHADER_TYPE::VS, &m_MeshBuffers.m_TangentBuffer.GetBuffer());
					m_DefaultRenderPass.BindBuffer("MeshEntryBuffer", D3D12::SHADER_TYPE::VS, &m_MeshBuffers.m_MeshEntryBuffer.GetBuffer());

					m_DefaultRenderPass.BindBuffer("InstanceBuffer", D3D12::SHADER_TYPE::VS, &m_StaticMeshFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("PointLightBuffer", D3D12::SHADER_TYPE::VS, &m_PointLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("SpotLightBuffer", D3D12::SHADER_TYPE::VS, &m_SpotLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("DirectionalLightBuffer", D3D12::SHADER_TYPE::VS, &m_DirectionalLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindConstant("PixelShaderConstantsBuffer", D3D12::SHADER_TYPE::PS, &pixelShaderPassData, sizeof(pixelShaderPassData));

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

							// TODO: Change name from "CameraDataBuffer" to "VertexPerPassData"
							m_DefaultRenderPass.m_Pass.SetShaderResource(batchCommandList, "CameraDataBuffer", (void*)&vertexShaderPassData, sizeof(vertexShaderPassData), D3D12::SHADER_TYPE::VS);
							m_DefaultRenderPass.m_Pass.SetShaderResource(batchCommandList, "PerBatchConstantsBuffer", (void*)&vertexPerDrawData, sizeof(vertexPerDrawData), D3D12::SHADER_TYPE::VS);

							uint32_t numVertices = meshHandle.NumVertices;
							batchCommandList->DrawInstanced(numVertices, batchArray.InstanceData.size(), 0, 0);

							m_GraphicsQueue.ExecuteContext(*batchCommandContext.value().m_Context);
						}
					}
				}
			}
		}

		void RendererNew::Init(ID3D12Device* device, uint32_t bufferCount, const std::string& contentPath)
		{
			m_Device = device;
			m_BufferCount = bufferCount;
			m_ContentPath = contentPath;

			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));

			this->InitCommandRecording();
			this->InitDescriptorHeaps();
			this->InitFrameResources();
			this->InitSamplers();
			this->InitRenderPasses();
		}

		void RendererNew::InitCommandRecording()
		{
			m_GraphicsQueue.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_CommandContextAllocator.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT, 10);
		}


		void RendererNew::InitDescriptorHeaps()
		{
			m_ResourceHeap.Init(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);
			m_SamplerHeap.Init(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeap.Init(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeap.Init(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);
		}

		void RendererNew::InitFrameResources()
		{
			// TODO: Remove once the buffers auto-expand once they are full
			const uint64_t frameBufferSize = 10000;
			for (int i = 0; i < m_BufferCount; i++)
			{
				m_StaticMeshFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));
				
				m_PointLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));

				m_SpotLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));

				m_DirectionalLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferSize, m_ResourceRecycler));
			}

			// TODO: Change so these use 64bit ints
			m_FrameAllocator.Init(m_Device, sizeof(int32_t) * 100000, m_ResourceRecycler);
			m_MeshBuffers.m_PositionBuffer.Init(m_Device, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_UVBuffer.Init(m_Device, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_NormalBuffer.Init(m_Device, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_TangentBuffer.Init(m_Device, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_IndexBuffer.Init(m_Device, sizeof(int32_t) * 100000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshBuffers.m_MeshEntryBuffer.Init(m_Device, sizeof(int32_t) * 10000, m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
		}

		void RendererNew::InitSamplers()
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
			m_Device->CreateSampler(&SamplerDesc, m_AnisotropicSampler.GetCPUHandle());
		}

		void RendererNew::InitRenderPasses()
		{
			D3D12::Shader BasePassVS;
			BasePassVS.CompileFromFile(*m_Compiler.p, m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");

			D3D12::Shader BasePassPS;
			BasePassPS.CompileFromFile(*m_Compiler.p, m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

			D3D12::RenderPass Pass;
			Pass.Init(m_Device, BasePassVS, BasePassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT);
			m_DefaultRenderPass = RenderGraphPass(std::move(Pass));
		}

		void RendererNew::AllocateFreelistMesh(AssetNew::Mesh& mesh, ID3D12GraphicsCommandList* cmdList)
		{
			int allocSize = mesh.m_VertexData.Positions.size() * sizeof(mesh.m_VertexData.Positions.at(0)); // TODO: Indices might be larger...
			DS::FreelistAllocator::AllocationHandle allocHandle;

			m_MeshBuffers.m_PositionBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_PositionBuffer.Write(cmdList, m_FrameAllocator, allocHandle, mesh.m_VertexData.Positions.data());
			m_MeshBuffers.m_PositionAllocMap[mesh.m_AssetID] = std::move(allocHandle);

			m_MeshBuffers.m_NormalBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_NormalBuffer.Write(cmdList, m_FrameAllocator, allocHandle, mesh.m_VertexData.Normals.data());
			m_MeshBuffers.m_NormalAllocMap[mesh.m_AssetID] = std::move(allocHandle);

			m_MeshBuffers.m_UVBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_UVBuffer.Write(cmdList, m_FrameAllocator, allocHandle, mesh.m_VertexData.UVs.data());
			m_MeshBuffers.m_UVAllocMap[mesh.m_AssetID] = std::move(allocHandle);

			m_MeshBuffers.m_TangentBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_TangentBuffer.Write(cmdList, m_FrameAllocator, allocHandle, mesh.m_VertexData.Tangents.data());
			m_MeshBuffers.m_TangentAllocMap[mesh.m_AssetID] = std::move(allocHandle);

			m_MeshBuffers.m_IndexBuffer.Allocate(allocHandle, allocSize);
			m_MeshBuffers.m_IndexBuffer.Write(cmdList, m_FrameAllocator, allocHandle, mesh.m_VertexData.Indices.data());
			m_MeshBuffers.m_IndexAllocMap[mesh.m_AssetID] = std::move(allocHandle);
		}

		void RendererNew::Upload(AssetNew::Mesh& mesh)
		{
			// hitta plats i vertex buffrarna och i MeshHandle
			if (mesh.m_VertexData.Positions.size() == 0)
			{
				return;
			}

			// TODO: implement resize with this cmdlist, otherwise it isnt needed
			auto cmdList = m_CommandContextAllocator.GetContext()->m_Context->GetCommandList();

			if (mesh.m_RenderID == std::numeric_limits<AssetNew::RenderID>::max())
			{
				// ladda upp datan
				this->AllocateFreelistMesh(mesh, cmdList);

				DS::FreelistAllocator::AllocationHandle allocHandle;
				MeshHandle meshHandle;
				meshHandle.StartIndex = m_MeshBuffers.m_IndexAllocMap[mesh.m_AssetID].GetStartOffset() / sizeof(mesh.m_VertexData.Positions.at(0));
				meshHandle.NumVertices = mesh.m_VertexData.Indices.size();
				m_MeshBuffers.m_MeshEntryBuffer.Allocate(allocHandle, sizeof(MeshHandle));
				m_MeshBuffers.m_MeshEntryBuffer.Write(cmdList, m_FrameAllocator, allocHandle, &meshHandle);
				m_MeshBuffers.m_MeshEntryAllocMap[mesh.m_AssetID] = std::move(allocHandle);

				// sätt renderID till MeshHandle index
				mesh.m_RenderID = allocHandle.GetStartOffset() / sizeof(MeshHandle);
			}
			else
			{
				// free-a upp platsen i freelistarna för vertex buffrarna
				m_MeshBuffers.m_PositionAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_UVAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_NormalAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_TangentAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_IndexAllocMap.erase(mesh.m_AssetID);

				this->AllocateFreelistMesh(mesh, cmdList);

				// updatera MeshHandle för meshens renderID
				MeshHandle meshHandle;
				meshHandle.StartIndex = m_MeshBuffers.m_IndexAllocMap[mesh.m_AssetID].GetStartOffset() / sizeof(mesh.m_VertexData.Positions.at(0));
				meshHandle.NumVertices = mesh.m_VertexData.Indices.size();
				m_MeshBuffers.m_MeshEntryBuffer.Write(cmdList, m_FrameAllocator, m_MeshBuffers.m_MeshEntryAllocMap[mesh.m_AssetID], &meshHandle);
			}
		}

		void RendererNew::Upload(AssetNew::Material& material)
		{
			std::weak_ptr<AssetNew::Texture> albedoTexture = material.m_Data.AlbedoTexture;
			std::weak_ptr<AssetNew::Texture> normalTexture = material.m_Data.NormalMap;

			DS::FreelistAllocator::AllocationHandle allocHandle;

			struct MaterialData
			{
				int32_t AlbedoIndex;
				int32_t NormalIndex;
			};
			MaterialData materialData;

			if (albedoTexture.expired() || albedoTexture.lock()->m_RenderID == std::numeric_limits<AssetNew::RenderID>::max())
			{
				materialData.AlbedoIndex = -1;
			}
			else
			{
				materialData.AlbedoIndex = albedoTexture.lock()->m_RenderID;
			}

			if (normalTexture.expired() || normalTexture.lock()->m_RenderID == std::numeric_limits<AssetNew::RenderID>::max())
			{
				materialData.NormalIndex = -1;
			}
			else
			{
				materialData.NormalIndex = normalTexture.lock()->m_RenderID;
			}

			if (material.m_RenderID == std::numeric_limits<AssetNew::RenderID>::max())
			{
				m_MaterialBuffer.Allocate(allocHandle, sizeof(materialData));
				material.m_RenderID = allocHandle.GetStartOffset() / sizeof(materialData);
				m_MaterialAllocMap[material.m_AssetID] = std::move(allocHandle);
			}
				
			m_MaterialBuffer.Write(nullptr, m_FrameAllocator, m_MaterialAllocMap[material.m_AssetID], &materialData);
		}

		void RendererNew::Upload(AssetNew::Texture& texture)
		{
			TextureRenderAsset assetData;
			assetData.m_Resource = D3D12::GPUTexture(m_Device,
				DXM::Vector3(texture.m_Data.Width, texture.m_Data.Height, 1),
				DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
				m_ResourceRecycler,
				1);

			// TODO: Be sure the old texture isnt accessed when/after the same descriptor index is reallocated
				// We need to use the same descriptor index since we need to keep the renderID valid
			assetData.m_Srv.Init(m_Device, m_ResourceHeap.GetDescriptor(), assetData.m_Resource, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

			if (texture.m_RenderID == std::numeric_limits<AssetNew::RenderID>::max())
			{
				texture.m_RenderID = assetData.m_Srv.GetDescriptorIndex();
			}

			// Upload texel data
			const uint64_t StagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(assetData.m_Resource.GetResource(), 0, 1));
			D3D12::GPUBuffer StagingBuffer;
			StagingBuffer.Init(m_Device, D3D12_HEAP_TYPE_UPLOAD, StagingBufferSize, m_ResourceRecycler);

			std::vector<D3D12::ResourceTransitionBundles> Bundle;
			Bundle.push_back({});
			Bundle.at(0).ResourcePtr = assetData.m_Resource.GetResource();
			Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COMMON;
			Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

			auto CmdContext = m_CommandContextAllocator.GetContext();
			if (!CmdContext.has_value())
			{
				throw std::runtime_error("No more command contexts");
			}
			ID3D12GraphicsCommandList* CmdList = CmdContext->m_Context->GetCommandList();
			D3D12::TransitionResources(CmdList, Bundle);

			D3D12_SUBRESOURCE_DATA SubresourceData{};
			SubresourceData.pData = texture.m_Data.TexelData.data();
			SubresourceData.RowPitch = /*roundUp(*/texture.m_Data.Width * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
			SubresourceData.SlicePitch = SubresourceData.RowPitch * texture.m_Data.Height;

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


			m_TextureAllocMap[texture.m_AssetID] = std::move(assetData);
		}

		void RendererNew::Deload(AssetNew::Mesh& mesh)
		{
			if (mesh.m_RenderID != std::numeric_limits<AssetNew::RenderID>::max())
			{
				mesh.m_RenderID = std::numeric_limits<AssetNew::RenderID>::max();
				m_MeshBuffers.m_PositionAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_UVAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_NormalAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_TangentAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_IndexAllocMap.erase(mesh.m_AssetID);
				m_MeshBuffers.m_MeshEntryAllocMap.erase(mesh.m_AssetID);
			}
		}

		void RendererNew::Deload(AssetNew::Material& material)
		{
			if (material.m_RenderID != std::numeric_limits<AssetNew::RenderID>::max())
			{
				material.m_RenderID = std::numeric_limits<AssetNew::RenderID>::max();
				m_MaterialAllocMap.erase(material.m_AssetID);
			}
		}

		void RendererNew::Deload(AssetNew::Texture& texture)
		{
			if (texture.m_RenderID != std::numeric_limits<AssetNew::RenderID>::max())
			{
				texture.m_RenderID = std::numeric_limits<AssetNew::RenderID>::max();

				// TODO: Remove it after its last usage so we dont create a new view with the same descriptor index while its still accessed on the gpu
				m_TextureAllocMap.erase(texture.m_AssetID);
			}
		}

		void Renderer::Init(ID3D12Device* Device, uint32_t BufferCount, const std::string& ContentPath)
		{
			m_ContentPath = ContentPath;
			m_Device = Device;
			m_BufferCount = BufferCount;

			m_ResourceHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);
			m_SamplerHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);

			m_GraphicsQueue.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			//m_GraphicsCommandContext.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			//m_PackedGPULookupBufferUpdateContext.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

			m_GraphicsCmdContextAllocator.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT, 10);

			m_AssetManager = std::make_unique<Asset::RenderAssetManager>();

			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));

			SetupRenderPipeline();

			InitPrimitiveBatchPipeline();
		}

		void Renderer::BeginFrame()
		{
			m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_BufferCount);
		}

		

		void Renderer::RenderPrimitiveBatch(D3D12::CommandContext& CmdContext, const D3D12::RenderTargetView& ColorView, const PrimitiveBatch& Batch, const Scene::Scene::Camera& Camera)
		{
			if (Batch.IsDrawable())
			{
				ID3D12GraphicsCommandList* CmdList = CmdContext.GetCommandList();

				m_BatchVertexBuffer.Write(CmdList, Batch.GetData(), Batch.GetNumBytes());

				this->GetFrameAllocator().GetCommandContext().StopRecording();
				m_GraphicsQueue.ExecuteContext(this->GetFrameAllocator().GetCommandContext());

				ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap.GetDescriptorHeap(), m_SamplerHeap.GetDescriptorHeap() };
				CmdList->SetDescriptorHeaps(2, Heaps);

				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles = { ColorView.GetDescriptorHandle() };

				D3D12::RenderPass* Pass = nullptr;
				const PrimitiveBatch::RenderLayer RenderLayer = Batch.GetLayer();
				const D3D_PRIMITIVE_TOPOLOGY PrimitiveType = Batch.GetPrimitiveType();
				if (RenderLayer == PrimitiveBatch::RenderLayer::DEPTH)
				{
					if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_POINTLIST)
					{
						Pass = &m_BatchPassDepthP;
					}
					else if ((PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINELIST) || (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP))
					{
						Pass = &m_BatchPassDepthL;
					}
					else if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
					{
						Pass = &m_BatchPassDepthT;
					}
				}
				else if (RenderLayer == PrimitiveBatch::RenderLayer::NO_DEPTH)
				{
					if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_POINTLIST)
					{
						Pass = &m_BatchPassNoDepthP;
					}
					else if ((PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINELIST) || (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP))
					{
						Pass = &m_BatchPassNoDepthL;
					}
					else if (PrimitiveType == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
					{
						Pass = &m_BatchPassNoDepthT;
					}
				}

				CmdList->SetPipelineState(Pass->GetPipelineState());
				CmdList->SetGraphicsRootSignature(Pass->GetRootSignature());

				CmdList->IASetPrimitiveTopology(PrimitiveType);

				CmdList->RSSetViewports(1, &Camera.Viewport);
				CmdList->RSSetScissorRects(1, &Camera.ScizzorRect);

				struct VertexShaderConstants
				{
					DXM::Matrix ViewProjectionMatrix;
				};

				VertexShaderConstants VSConstants;
				VSConstants.ViewProjectionMatrix = Camera.ViewMatrix * Camera.ProjMatrix;

				Pass->SetShaderResource(CmdList, "CameraDataBuffer", &VSConstants, sizeof(VertexShaderConstants), D3D12::SHADER_TYPE::VS);

				D3D12_VERTEX_BUFFER_VIEW VBView;
				VBView.BufferLocation = m_BatchVertexBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress();
				VBView.SizeInBytes = Batch.GetNumBytes();
				VBView.StrideInBytes = sizeof(PrimitiveBatch::Point);
				CmdList->IASetVertexBuffers(0, 1, &VBView);

				CmdList->DrawInstanced(static_cast<UINT>(Batch.GetNumPoints()), 1, 0, 0);

				CmdContext.StopRecording();
				m_GraphicsQueue.ExecuteContext(CmdContext);
			}
		}

		void Renderer::EndFrame()
		{
			if (m_FrameCount % m_BufferCount == 0)
			{
				m_GraphicsQueue.FlushCommands();
				m_ResourceRecycler.ReleaseResources();
				m_GraphicsCmdContextAllocator.FreeCommandBuffers();

				for (D3D12::LinearFrameAllocator& Allocator : m_FrameAllocators)
				{
					Allocator.GetCommandContext().FreeCommandBuffer();
				}
			}

			m_AssetManager->ClearGarbage<Asset::Mesh>();
			m_AssetManager->ClearGarbage<Asset::Texture>();
			m_AssetManager->ClearGarbage<Asset::Material>();

			m_FrameAllocators[m_FrameIndex].Reset();
			m_FrameCount++;
		}

		void Renderer::SetupRenderPipeline()
		{
			m_FrameAllocators.resize(m_BufferCount);
			for (int BufferIndex = 0; BufferIndex < m_BufferCount; BufferIndex++)
			{
				m_FrameAllocators[BufferIndex].Init(m_Device, sizeof(int32_t) * 1, m_ResourceRecycler);
			}

			m_VertexBuffer.Init(m_Device, sizeof(Asset::VertexData), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_IndexBuffer.Init(m_Device,sizeof(uint32_t), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshEntryBuffer.Init(m_Device, sizeof(Asset::MeshEntry), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MaterialBuffer.Init(m_Device, sizeof(Asset::MaterialData::MaterialRenderData), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);

			m_DefaultSamplerDescriptor = m_SamplerHeap.GetDescriptor();
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
			m_Device->CreateSampler(&SamplerDesc, m_DefaultSamplerDescriptor.GetCPUHandle());

			this->SetupRenderGraph();
		}

		void Renderer::InitPrimitiveBatchPipeline()
		{
			m_BatchVertexBuffer.Init(m_Device, D3D12_HEAP_TYPE_UPLOAD, 2, m_ResourceRecycler);

			D3D12::Shader PassVS;
			PassVS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "PrimitiveBatch.vs.hlsl");

			D3D12::Shader PassPS;
			PassPS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "PrimitiveBatch.ps.hlsl");

			m_BatchPassDepthP.Init(m_Device, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
			m_BatchPassNoDepthP.Init(m_Device, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
			m_BatchPassDepthL.Init(m_Device, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
			m_BatchPassNoDepthL.Init(m_Device, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
			m_BatchPassDepthT.Init(m_Device, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			m_BatchPassNoDepthT.Init(m_Device, PassVS, PassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		}

		void Renderer::CopyRenderSurfaceToTexture(D3D12::CommandContext& CmdContext, ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture)
		{
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
		}
	}
}