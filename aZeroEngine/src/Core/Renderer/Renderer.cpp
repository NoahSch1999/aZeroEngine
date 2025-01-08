#include "Renderer.h"
#include "Core/Scene/Scene.h"

namespace aZero
{
	namespace Rendering
	{

		struct StaticMeshBatches
		{
			struct PerInstanceData
			{
				DXM::Matrix WorldMatrix;
			};

			struct BatchInfo
			{
				uint32_t StartInstanceOffset;
				uint32_t NumVertices;
				std::vector<PerInstanceData> InstanceData;
			};

			std::unordered_map<uint32_t, std::unordered_map<uint32_t, BatchInfo>> Batches;
		};

		void SetRenderPass(ID3D12GraphicsCommandList* CmdList, D3D12::RenderPass& Pass)
		{
			CmdList->SetPipelineState(Pass.GetPipelineState());

			if (Pass.IsComputePass())
			{
				CmdList->SetComputeRootSignature(Pass.GetRootSignature());
			}
			else
			{
				CmdList->SetGraphicsRootSignature(Pass.GetRootSignature());

				const D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = Pass.GetTopologyType();
				switch (TopologyType)
				{
				case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
				{
					CmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					break;
				}
				case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
				{
					CmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST);
					break;
				}
				case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
				{
					CmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
					break;
				}
				default:
				{
					DEBUG_PRINT("Input pass has an invalid topology type");
				}
				}
			}
		}

		void Renderer::Init(ID3D12Device* Device, const DXM::Vector2& RenderResolution, uint32_t BufferCount)
		{
			m_Device = Device;
			m_RenderResolution = RenderResolution;
			this->SetBufferCount(BufferCount);

			m_ResourceHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);
			m_SamplerHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);

			m_GraphicsQueue.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_GraphicsCommandContext.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_PackedGPULookupBufferUpdateContext.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

			m_AssetManager = std::make_unique<Asset::RenderAssetManager>();
			
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));

			SetupRenderPipeline();
		}

		void Renderer::BeginFrame()
		{
			m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_BufferCount);
			this->GetFrameAllocator().GetCommandContext().StartRecording();
		}

		void Renderer::GetRelevantStaticMeshes(
			NewScene::Scene& Scene,
			const NewScene::Scene::Camera& Camera,
			StaticMeshBatches& OutStaticMeshBatches)
		{
			// TODO: Perform all gpu writes in the loops to avoid having to iterate the arrays again
			//		Ex. ::WorldMatrices should be a upload heap buffer 
			
			// TODO: This should be done on a list of primitives after octree etc
			for (const auto& StaticMesh : Scene.GetPrimitives<NewScene::Scene::StaticMesh>())
			{
				// TODO: If passed frustrum culling using StaticMesh.Bounds and Camera
				if (true)
				{
					const uint32_t MeshIndex = m_AssetManager->GetRenderHandle(*StaticMesh.Mesh.get()).value()->MeshEntryAllocHandle.GetStartOffset() / sizeof(Asset::MeshEntry::GPUData);
					const uint32_t MaterialIndex = m_AssetManager->GetRenderHandle(*StaticMesh.Material.get()).value()->MaterialAllocHandle.GetStartOffset() / sizeof(Asset::MaterialData::MaterialRenderData);

					auto& Batch = OutStaticMeshBatches.Batches[MaterialIndex][MeshIndex];
					Batch.NumVertices = StaticMesh.Mesh->GetData().Indices.size();

					StaticMeshBatches::PerInstanceData InstanceData;
					InstanceData.WorldMatrix = StaticMesh.WorldMatrix;
					Batch.InstanceData.emplace_back(std::move(InstanceData));
				}
			}

			// TODO: Handle case where scene doesnt have for frame index
			std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetPrimitives<NewScene::Scene::StaticMesh>().m_RenderBuffers;
			if (RenderBuffers.size() != m_BufferCount)
			{
				RenderBuffers.resize(m_BufferCount);
			}

			D3D12::LinearBuffer& InstanceBuffer = RenderBuffers.at(m_FrameIndex);
			InstanceBuffer.Reset();
			if (!InstanceBuffer.IsInitalized())
			{
				InstanceBuffer.Init(m_Device,
					D3D12::GPUBuffer::RWPROPERTY::CPUWRITE,
					sizeof(StaticMeshBatches::PerInstanceData) * std::max(static_cast<size_t>(1), Scene.GetPrimitives<NewScene::Scene::StaticMesh>().size()),
					&m_ResourceRecycler
				);
			}
			else
			{
				// Get size and resize if needed / handle shrinking at some point
			}

			uint32_t InstanceStartOffset = 0;
			for (auto& [MaterialIndex, MeshToBatchMap] : OutStaticMeshBatches.Batches)
			{
				for (auto& [MeshIndex, Batch] : MeshToBatchMap)
				{
					Batch.StartInstanceOffset = InstanceStartOffset;
					for (auto& InstanceData : Batch.InstanceData)
					{
						InstanceBuffer.Write((void*)&InstanceData, sizeof(StaticMeshBatches::PerInstanceData));
					}
					InstanceStartOffset += Batch.InstanceData.size();
				}
			}
		}

		void Renderer::GetRelevantPointLights(NewScene::Scene& Scene, const NewScene::Scene::Camera& Camera, uint32_t& NumPointLights)
		{
			// TODO: Handle case where scene doesnt have for frame index
			std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetPrimitives<NewScene::Scene::PointLight>().m_RenderBuffers;
			if (RenderBuffers.size() != m_BufferCount)
			{
				RenderBuffers.resize(m_BufferCount);
			}

			D3D12::LinearBuffer& PointLightBuffer = RenderBuffers.at(m_FrameIndex);
			PointLightBuffer.Reset();
			if (!PointLightBuffer.IsInitalized())
			{
				PointLightBuffer.Init(m_Device,
					D3D12::GPUBuffer::RWPROPERTY::CPUWRITE,
					sizeof(NewScene::Scene::PointLight) * std::max(static_cast<size_t>(1), Scene.GetPrimitives<NewScene::Scene::PointLight>().size()),
					&m_ResourceRecycler
				);
			}
			else
			{
				// Get size and resize if needed / handle shrinking at some point
			}

			for (const auto& PointLight : Scene.GetPrimitives<NewScene::Scene::PointLight>())
			{
				// TODO: If passed frustrum culling using PointLight.Range and Camera
				if (true)
				{
					PointLightBuffer.Write((void*)&PointLight, sizeof(NewScene::Scene::PointLight));
				}
			}

			NumPointLights = PointLightBuffer.GetOffset() / sizeof(NewScene::Scene::PointLight);
		}

		void Renderer::Render(NewScene::Scene& Scene, std::shared_ptr<Window::RenderWindow> Window)
		{
			m_FrameWindows.insert(Window);

			// TODO: Rework this and make also call the scene's linear alloc if it has
			this->RecordAssetManagerCommands();

			this->ClearRenderSurfaces();

			for (auto& Camera : Scene.GetPrimitives<NewScene::Scene::Camera>())
			{
				if (Camera.IsActive)
				{
					StaticMeshBatches AllStaticMeshBatches;
					// SkeletalMeshBatches AllSkeletalMeshBatches;
					uint32_t NumPointLights;

					this->GetRelevantStaticMeshes(Scene, Camera, AllStaticMeshBatches);

					this->GetRelevantSkeletalMeshes(Scene, Camera /*, AllSkeletalMeshBatches*/);

					this->GetRelevantPointLights(Scene, Camera, NumPointLights);

					this->RenderStaticMeshes(Scene, Camera, AllStaticMeshBatches, NumPointLights);

					this->RenderSkeletalMeshes(Scene, Camera /*, AllSkeletalMeshBatches, NumPointLights*/);
				}
			}

			// TODO: Copy render surface to window back buffer
				// Upscale/downscale to match window resolution
				// Make the render surface be automatically given from the pipeline etc once some kind of RDG is implemented
			ID3D12Resource* Backbuffer = Window->GetBackBuffer(m_FrameIndex);
			if (Backbuffer)
			{
				this->CopyRenderSurfaceToTexture(Backbuffer);
			}

			// TODO: Remove these and use a smarter recording approach
			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
		}

		void Renderer::EndFrame()
		{
			// TODO: Try to avoid incrementing refcount while doing this
			for (std::shared_ptr<Window::RenderWindow> Window : m_FrameWindows)
			{
				Window->Present();
			}

			if (m_FrameCount % m_BufferCount == 0)
			{
				m_GraphicsQueue.FlushCommands();
				m_ResourceRecycler.ReleaseResources();
				m_GraphicsCommandContext.FreeCommandBuffer();
				m_PackedGPULookupBufferUpdateContext.FreeCommandBuffer();

				for (D3D12::LinearFrameAllocator& Allocator : m_FrameAllocators)
				{
					Allocator.GetCommandContext().FreeCommandBuffer();
				}
			}

			m_FrameWindows.clear();

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
				m_FrameAllocators[BufferIndex].Init(m_Device, 1000000 * sizeof(Asset::VertexData));
			}

			const uint64_t MaxVertices = 1000000;
			const uint64_t MaxMeshes = 1000;
			const uint64_t MaxMaterials = 1000;
			D3D12_SHADER_RESOURCE_VIEW_DESC MeshSRVDesc;
			MeshSRVDesc.Buffer.FirstElement = 0;
			MeshSRVDesc.Buffer.NumElements = MaxVertices;
			MeshSRVDesc.Buffer.StructureByteStride = sizeof(Asset::VertexData);
			MeshSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			MeshSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
			MeshSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			MeshSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			m_VertexBuffer.Init(m_Device, MaxVertices * sizeof(Asset::VertexData), &m_ResourceRecycler, D3D12::GPUResource::GPUONLY);
			m_IndexBuffer.Init(m_Device, MaxVertices * sizeof(uint32_t), &m_ResourceRecycler, D3D12::GPUResource::GPUONLY);
			m_MeshEntryBuffer.Init(m_Device, MaxMeshes * sizeof(Asset::MeshEntry), &m_ResourceRecycler, D3D12::GPUResource::GPUONLY);
			m_MaterialBuffer.Init(m_Device, MaxMeshes * sizeof(Asset::MaterialData::MaterialRenderData), &m_ResourceRecycler, D3D12::GPUResource::GPUONLY);

			m_SamplerHeap.GetDescriptor(m_DefaultSamplerDescriptor);
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

			m_DSVClearColor.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_DSVClearColor.DepthStencil.Depth = 1.f;
			m_DSVClearColor.DepthStencil.Stencil = 0;
			
			m_SceneDepthTexture.Init(
				m_Device,
				DXM::Vector3(m_RenderResolution.x, m_RenderResolution.y, 1),
				DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				1,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				m_DSVClearColor,
				&m_ResourceRecycler
			);

			m_DSVHeap.GetDescriptor(m_SceneDepthTextureDSV);

			D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
			DsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			DsvDesc.Texture2D.MipSlice = 0;
			DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			m_Device->CreateDepthStencilView(m_SceneDepthTexture.GetResource(), &DsvDesc, m_SceneDepthTextureDSV.GetCPUHandle());

			m_RTVClearColor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			m_RTVClearColor.Color[0] = 0.2f;
			m_RTVClearColor.Color[1] = 0.2f;
			m_RTVClearColor.Color[2] = 0.2f;
			m_RTVClearColor.Color[3] = 1.f;

			m_FinalRenderSurface.Init(
				m_Device,
				DXM::Vector3(m_RenderResolution.x, m_RenderResolution.y, 1),
				m_RTVClearColor.Format,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				1,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				m_RTVClearColor,
				&m_ResourceRecycler
			);

			D3D12_RENDER_TARGET_VIEW_DESC RtvDesc;
			RtvDesc.Format = m_FinalRenderSurface.GetResource()->GetDesc().Format;
			RtvDesc.Texture2D.MipSlice = 0;
			RtvDesc.Texture2D.PlaneSlice = 0;
			RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			m_RTVHeap.GetDescriptor(m_FinalRenderSurfaceRTV);
			m_Device->CreateRenderTargetView(m_FinalRenderSurface.GetResource(), &RtvDesc, m_FinalRenderSurfaceRTV.GetCPUHandle());

			{
				D3D12::Shader BasePassVS;
				BasePassVS.CompileFromFile(this->GetCompiler(), SHADER_SRC_PATH("BasePass.vs"));

				D3D12::Shader BasePassPS;
				BasePassPS.CompileFromFile(this->GetCompiler(), SHADER_SRC_PATH("BasePass.ps"));

				Pass.Init(m_Device, BasePassVS, BasePassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_D24_UNORM_S8_UINT);
			}
		}

		void Renderer::RecordAssetManagerCommands()
		{
			D3D12::LinearFrameAllocator& FrameAllocator = m_FrameAllocators[m_FrameIndex];

			FrameAllocator.RecordAllocations();
			m_GraphicsQueue.ExecuteContext(FrameAllocator.GetCommandContext());
		}

		void Renderer::RenderStaticMeshes(NewScene::Scene& InScene, const NewScene::Scene::Camera& Camera,
			const StaticMeshBatches& StaticMeshBatches, uint32_t NumPointLights)
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap.GetDescriptorHeap(), m_SamplerHeap.GetDescriptorHeap() };
			CmdList->SetDescriptorHeaps(2, Heaps);

			SetRenderPass(CmdList, Pass);

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles = { m_FinalRenderSurfaceRTV.GetCPUHandle() };
			D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = m_SceneDepthTextureDSV.GetCPUHandle();
			CmdList->OMSetRenderTargets(static_cast<UINT>(RTVHandles.size()), RTVHandles.data(), false, &DSVHandle);

			CmdList->RSSetViewports(1, &Camera.Viewport);
			CmdList->RSSetScissorRects(1, &Camera.ScizzorRect);

			{
				struct VertexShaderConstants
				{
					DXM::Matrix ViewProjectionMatrix;
				};

				struct PixelShaderConstants
				{
					int SamplerIndex;
				};

				VertexShaderConstants VSConstants;
				VSConstants.ViewProjectionMatrix = Camera.ViewProjectionMatrix;

				Pass.SetShaderResource(CmdList, "CameraDataBuffer", &VSConstants, sizeof(VertexShaderConstants), D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "VertexBuffer",
					m_VertexBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "IndexBuffer",
					m_IndexBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "MeshEntries",
					m_MeshEntryBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::VS);


				Pass.SetShaderResource(CmdList, "MaterialBuffer",
					m_MaterialBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::PS
				);

				// TODO: Set, but need to init sampler etc first
				PixelShaderConstants PSConstants;
				PSConstants.SamplerIndex = 0;
				//Pass.SetShaderResource(CmdList, "PixelShaderConstants", &PSConstants, sizeof(PixelShaderConstants), D3D12::SHADER_TYPE::PS);

				Pass.SetShaderResource(CmdList, "InstanceBuffer", 
					InScene.m_RenderProxy.GetPrimitives<NewScene::Scene::StaticMesh>().m_RenderBuffers.at(m_FrameIndex).GetVirtualAddress(), D3D12::SHADER_TYPE::VS);

				// TODO: Do point light stuff using "NumPointLights"

				struct PerBatchConstantsVS
				{
					unsigned int StartInstanceOffset;
					unsigned int MeshEntryIndex;
				};

				for (auto& [MaterialIndex, MeshToBatchMap] : StaticMeshBatches.Batches)
				{
					for (auto& [MeshIndex, Batch] : MeshToBatchMap)
					{
						PerBatchConstantsVS VSConstants;
						VSConstants.MeshEntryIndex = MeshIndex;
						VSConstants.StartInstanceOffset = Batch.StartInstanceOffset;

						const uint32_t NumInstances = Batch.InstanceData.size();
						Pass.SetShaderResource(CmdList, "PerBatchConstantsBuffer", (void*)&VSConstants, sizeof(VSConstants), D3D12::SHADER_TYPE::VS);
						Pass.SetShaderResource(CmdList, "PerBatchConstantsBuffer", (void*)&MaterialIndex, sizeof(MaterialIndex), D3D12::SHADER_TYPE::PS);
	
						CmdList->DrawInstanced(static_cast<UINT>(Batch.NumVertices), NumInstances, 0, 0);
					}
				}
			}
		}

		void Renderer::ClearRenderSurfaces()
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			CmdList->ClearRenderTargetView(m_FinalRenderSurfaceRTV.GetCPUHandle(), m_RTVClearColor.Color, 0, NULL);
			CmdList->ClearDepthStencilView(
					m_SceneDepthTextureDSV.GetCPUHandle(),
					D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
					m_DSVClearColor.DepthStencil.Depth, m_DSVClearColor.DepthStencil.Stencil,
					0, NULL);
		}

		void Renderer::CopyRenderSurfaceToTexture(ID3D12Resource* TargetTexture)
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			std::vector<D3D12::ResourceTransitionBundles> PreCopyBarriers;
			PreCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, TargetTexture });
			PreCopyBarriers.push_back({ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE, m_FinalRenderSurface.GetResource() });
			D3D12::TransitionResources(CmdList, PreCopyBarriers);

			CmdList->CopyResource(TargetTexture, m_FinalRenderSurface.GetResource());

			std::vector<D3D12::ResourceTransitionBundles> PostCopyBarriers;
			PostCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, TargetTexture });
			PostCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, m_FinalRenderSurface.GetResource() });
			D3D12::TransitionResources(CmdList, PostCopyBarriers);
		}
	}
}