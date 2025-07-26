#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "Renderer.hpp"
#include "scene/Scene.hpp"

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

			auto prepRenderCommandList = prepRenderCommandContext.value().m_Context->GetCommandList();

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

			// TODO: Try to reuse the allocated memory in-between frames
			
			// TODO: Create the allocator with the different mapped "upload heap" buffers
			LinearAllocator frameBatchAllocator(nullptr, 0);
			std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<StaticMeshInstanceData>>> batches;

			LinearAllocator framePointLightAllocator(nullptr, 0);
			std::vector<Scene::SceneProxy::PointLight> pointLights;

			LinearAllocator frameSpotLightAllocator(nullptr, 0);
			std::vector<Scene::SceneProxy::SpotLight> spotLights;

			LinearAllocator frameDirectionalLightAllocator(nullptr, 0);
			std::vector<Scene::SceneProxy::DirectionalLight> directionalLights;

			const auto& sceneProxy = Scene.m_Proxy;
			for (const auto& camera : sceneProxy.m_Cameras.GetData())
			{
				for (const auto& staticMeshEntity : sceneProxy.m_StaticMeshes.GetData())
				{
					if (camera.m_Frustrum.Intersects(staticMeshEntity.m_BoundingSphere))
					{
						StaticMeshInstanceData instanceData;
						instanceData.Transform = staticMeshEntity.m_Transform;
						batches[staticMeshEntity.m_MeshIndex][staticMeshEntity.m_MaterialIndex].emplace_back();
					}
				}

				for (const auto& pointLight : sceneProxy.m_PointLights.GetData())
				{
					if (true/* TODO: Perform culling */)
					{
						pointLights.emplace_back(pointLight);
					}
				}

				for (const auto& spotLight : sceneProxy.m_SpotLights.GetData())
				{
					if (true/* TODO: Perform culling */)
					{
						spotLights.emplace_back(spotLight);
					}
				}

				LinearAllocator::Allocation pointLightAllocation = framePointLightAllocator.Append((void*)pointLights.data(), pointLights.size() * sizeof(decltype(pointLights)::value_type));
				LinearAllocator::Allocation spotLightAllocation = frameSpotLightAllocator.Append((void*)spotLights.data(), spotLights.size() * sizeof(decltype(spotLights)::value_type));
				LinearAllocator::Allocation directionalLightAllocation = frameDirectionalLightAllocator.Append((void*)sceneProxy.m_DirectionalLights.GetData().data(), sceneProxy.m_DirectionalLights.GetData().size() * sizeof(decltype(sceneProxy.m_DirectionalLights.GetData())::value_type));

				// TODO: This should be done per render pass, ex. depth pass -> geometry pass -> deffered light pass -> per camera post-process pass etc...
				//		Replace this once some kind of rendergraph is implemented.
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

					// TODO: Remove this type of binding...
					m_DefaultRenderPass.BindBuffer("InstanceBuffer", D3D12::SHADER_TYPE::VS, &m_StaticMeshFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("PointLightBuffer", D3D12::SHADER_TYPE::VS, &m_PointLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("SpotLightBuffer", D3D12::SHADER_TYPE::VS, &m_SpotLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindBuffer("DirectionalLightBuffer", D3D12::SHADER_TYPE::VS, &m_DirectionalLightFrameBuffers.at(m_FrameIndex));
					m_DefaultRenderPass.BindConstant("PixelShaderConstantsBuffer", D3D12::SHADER_TYPE::PS, &pixelShaderPassData, sizeof(pixelShaderPassData));

					for (const auto& [meshIndex, batchArrayMap] : batches)
					{
						for (const auto& [materialIndex, batchArray] : batchArrayMap)
						{
							// TODO: Handle situation if the allocator memory pool gets full
							LinearAllocator::Allocation batchAllocation = frameBatchAllocator.Append((void*)batchArray.data(), batchArray.size() * sizeof(decltype(batchArray)::value_type));
							const MeshHandle& meshHandle = m_MeshHandles.at(meshIndex);
							const MaterialHandle& materialHandle = m_MaterialHandle.at(meshIndex);

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
							vertexPerDrawData.meshStartOffset = meshHandle.Offset;
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
							batchCommandList->DrawInstanced(numVertices, batchArray.size(), 0, 0);

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
			constexpr uint64_t frameBufferStartCount = 10000;
			for (int i = 0; i < m_BufferCount; i++)
			{
				m_StaticMeshFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferStartCount, m_ResourceRecycler));

				m_PointLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferStartCount, m_ResourceRecycler));

				m_SpotLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferStartCount, m_ResourceRecycler));

				m_DirectionalLightFrameBuffers.emplace_back(D3D12::GPUBuffer(m_Device, D3D12_HEAP_TYPE_UPLOAD, frameBufferStartCount, m_ResourceRecycler));
			}
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
				// TODO: Fix
				//D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = m_SceneDepthTextureDSV.GetDescriptorHandle();
				//CmdList->OMSetRenderTargets(static_cast<UINT>(RTVHandles.size()), RTVHandles.data(), false, &DSVHandle);

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

			// TODO: Check func and alloc size etc
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