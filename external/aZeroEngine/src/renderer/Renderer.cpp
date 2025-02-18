#include "Renderer.hpp"
#include "scene/Scene.hpp"

namespace aZero
{
	namespace Rendering
	{
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

		void Renderer::Init(ID3D12Device* Device, const DXM::Vector2& RenderResolution, uint32_t BufferCount, const std::string& ContentPath)
		{
			m_ContentPath = ContentPath;
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

			InitPrimitiveBatchPipeline();
		}

		void Renderer::BeginFrame()
		{
			m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_BufferCount);
			this->GetFrameAllocator().GetCommandContext().StartRecording();
		}

		void Renderer::RenderPrimitiveBatch(const PrimitiveBatch& Batch, const Scene::Scene::Camera& Camera)
		{
			if (Batch.IsDrawable())
			{
				ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

				m_BatchVertexBuffer.Write(CmdList, Batch.GetData(), Batch.GetNumBytes());

				this->GetFrameAllocator().GetCommandContext().StopRecording();
				m_GraphicsQueue.ExecuteContext(this->GetFrameAllocator().GetCommandContext());

				ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap.GetDescriptorHeap(), m_SamplerHeap.GetDescriptorHeap() };
				CmdList->SetDescriptorHeaps(2, Heaps);

				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles = { m_FinalRenderSurfaceRTV.GetDescriptorHandle() };
				D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = m_SceneDepthTextureDSV.GetDescriptorHandle();
				CmdList->OMSetRenderTargets(static_cast<UINT>(RTVHandles.size()), RTVHandles.data(), false, &DSVHandle);

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

				m_GraphicsCommandContext.StopRecording();
				m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
			}
		}

		void Renderer::GetRelevantStaticMeshes(
			Scene::Scene& Scene,
			const DirectX::BoundingFrustum& Frustum,
			const DXM::Matrix& ViewMatrix,
			StaticMeshBatches& OutStaticMeshBatches)
		{
			for (const auto& StaticMesh : Scene.GetObjects<Scene::Scene::StaticMesh>())
			{
				const DirectX::BoundingSphere Sphere(DXM::Vector3::Transform(StaticMesh.DXBounds.Center, ViewMatrix), StaticMesh.DXBounds.Radius);
				if (Frustum.Intersects(Sphere))
				{
					uint32_t MeshIndex;
					uint32_t MaterialIndex;
					if (m_AssetManager->HasRenderHandle(*StaticMesh.Mesh.get()))
					{
						MeshIndex = m_AssetManager->GetRenderHandle(*StaticMesh.Mesh.get()).value()->MeshEntryAllocHandle.GetStartOffset() / sizeof(Asset::MeshEntry::GPUData);
					}
					else
					{
						continue;
					}

					if (m_AssetManager->HasRenderHandle(*StaticMesh.Material.get()))
					{
						MaterialIndex = m_AssetManager->GetRenderHandle(*StaticMesh.Material.get()).value()->MaterialAllocHandle.GetStartOffset() / sizeof(Asset::MaterialData::MaterialRenderData);
					}
					else
					{
						continue;
					}

					auto& Batch = OutStaticMeshBatches.Batches[MaterialIndex][MeshIndex];
					Batch.NumVertices = StaticMesh.Mesh->GetData().Indices.size();

					StaticMeshBatches::PerInstanceData InstanceData;
					InstanceData.WorldMatrix = StaticMesh.WorldMatrix;
					Batch.InstanceData.emplace_back(std::move(InstanceData));
				}
			}

			// TODO: Handle case where scene doesnt have buffer for frame index
			std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<Scene::Scene::StaticMesh>().m_RenderBuffers;
			if (RenderBuffers.size() != m_BufferCount)
			{
				RenderBuffers.resize(m_BufferCount);
			}

			D3D12::LinearBuffer& InstanceBuffer = RenderBuffers.at(m_FrameIndex);
			InstanceBuffer.Reset();
			if (!InstanceBuffer.IsInitalized())
			{
				InstanceBuffer.Init(m_Device,
					D3D12_HEAP_TYPE_UPLOAD,
					sizeof(StaticMeshBatches::PerInstanceData) * std::max(static_cast<size_t>(1), Scene.GetObjects<Scene::Scene::StaticMesh>().size()),
					m_ResourceRecycler
				);
			}

			// TODO: Try making this purely vectors
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();
			uint32_t InstanceStartOffset = 0;
			for (auto& [MaterialIndex, MeshToBatchMap] : OutStaticMeshBatches.Batches)
			{
				for (auto& [MeshIndex, Batch] : MeshToBatchMap)
				{
					Batch.StartInstanceOffset = InstanceStartOffset;
					for (auto& InstanceData : Batch.InstanceData)
					{
						InstanceBuffer.Write(CmdList, (void*)&InstanceData, sizeof(StaticMeshBatches::PerInstanceData));
					}
					InstanceStartOffset += Batch.InstanceData.size();
				}
			}
			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
		}

		void Renderer::GetDirectionalLights(Scene::Scene& Scene,
			uint32_t& NumDirectionalLights)
		{
			// TODO: Handle case where scene doesnt have for frame index
			std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<DirectionalLightData>().m_RenderBuffers;
			if (RenderBuffers.size() != m_BufferCount)
			{
				RenderBuffers.resize(m_BufferCount);
			}

			D3D12::LinearBuffer& DirLightBuffer = RenderBuffers.at(m_FrameIndex);
			DirLightBuffer.Reset();
			if (!DirLightBuffer.IsInitalized())
			{
				DirLightBuffer.Init(m_Device,
					D3D12_HEAP_TYPE_UPLOAD,
					sizeof(DirectionalLightData) * std::max(static_cast<size_t>(1), Scene.GetObjects<DirectionalLightData>().size()),
					m_ResourceRecycler
				);
			}

			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();
			for (const auto& PointLight : Scene.GetObjects<DirectionalLightData>())
			{
				DirLightBuffer.Write(CmdList, (void*)&PointLight, sizeof(DirectionalLightData));
			}
			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);

			NumDirectionalLights = DirLightBuffer.GetOffset() / sizeof(DirectionalLightData);
		}

		void Renderer::GetRelevantPointLights(Scene::Scene& Scene,
			const DirectX::BoundingFrustum& Frustum,
			const DXM::Matrix& ViewMatrix,
			uint32_t& NumPointLights)
		{
			// TODO: Handle case where scene doesnt have for frame index
			std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<PointLightData>().m_RenderBuffers;
			if (RenderBuffers.size() != m_BufferCount)
			{
				RenderBuffers.resize(m_BufferCount);
			}

			D3D12::LinearBuffer& PointLightBuffer = RenderBuffers.at(m_FrameIndex);
			PointLightBuffer.Reset();
			if (!PointLightBuffer.IsInitalized())
			{
				PointLightBuffer.Init(m_Device,
					D3D12_HEAP_TYPE_UPLOAD,
					sizeof(PointLightData) * std::max(static_cast<size_t>(1), Scene.GetObjects<PointLightData>().size()),
					m_ResourceRecycler
				);
			}

			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			for (const auto& PointLight : Scene.GetObjects<PointLightData>())
			{
				// TODO: Add culling
				/*const DirectX::BoundingSphere Sphere(DXM::Vector3::Transform(PointLight.Position, ViewMatrix), PointLight.Range);
				if (Frustum.Intersects(Sphere))
				{*/
				PointLightBuffer.Write(CmdList, (void*)&PointLight, sizeof(PointLightData));
				//}
			}

			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);

			NumPointLights = PointLightBuffer.GetOffset() / sizeof(PointLightData);
		}

		void Renderer::GetRelevantSpotLights(Scene::Scene& Scene,
			const DirectX::BoundingFrustum& Frustum,
			const DXM::Matrix& ViewMatrix,
			uint32_t& NumSpotLights)
		{
			// TODO: Handle case where scene doesnt have for frame index
			std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<SpotLightData>().m_RenderBuffers;
			if (RenderBuffers.size() != m_BufferCount)
			{
				RenderBuffers.resize(m_BufferCount);
			}

			D3D12::LinearBuffer& SpotLightBuffer = RenderBuffers.at(m_FrameIndex);
			SpotLightBuffer.Reset();
			if (!SpotLightBuffer.IsInitalized())
			{
				SpotLightBuffer.Init(m_Device,
					D3D12_HEAP_TYPE_UPLOAD,
					sizeof(SpotLightData) * std::max(static_cast<size_t>(1), Scene.GetObjects<SpotLightData>().size()),
					m_ResourceRecycler
				);
			}

			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			for (const auto& PointLight : Scene.GetObjects<SpotLightData>())
			{
				// TODO: Add culling
				SpotLightBuffer.Write(CmdList, (void*)&PointLight, sizeof(SpotLightData));
			}

			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);

			NumSpotLights = SpotLightBuffer.GetOffset() / sizeof(SpotLightData);
		}

		void Renderer::Render(Scene::Scene& Scene, const std::vector<PrimitiveBatch*>& Batches, std::shared_ptr<Window::RenderWindow> Window)
		{
			Window->WaitOnSwapchain();

			m_FrameWindows.insert(Window);

			// TODO: Rework this and make also call the scene's linear alloc if it has
			this->RecordAssetManagerCommands();

			this->ClearRenderSurfaces();

			uint32_t NumDirectionalLights;
			this->GetDirectionalLights(Scene, NumDirectionalLights);

			for (auto& Camera : Scene.GetObjects<Scene::Scene::Camera>())
			{
				if (Camera.IsActive)
				{
					DirectX::BoundingFrustum Frustum = DirectX::BoundingFrustum(Camera.ProjMatrix, true);

					uint32_t NumPointLights;
					this->GetRelevantPointLights(Scene, Frustum, Camera.ViewMatrix, NumPointLights);

					uint32_t NumSpotLights;
					this->GetRelevantSpotLights(Scene, Frustum, Camera.ViewMatrix, NumSpotLights);

					StaticMeshBatches AllStaticMeshBatches;
					this->GetRelevantStaticMeshes(Scene, Frustum, Camera.ViewMatrix, AllStaticMeshBatches);

					this->PrepareGraphForScene(Scene);

					const bool Succeeded = m_RenderGraph.Execute(m_GraphicsQueue, m_GraphicsCommandContext, Camera, AllStaticMeshBatches, NumDirectionalLights, NumPointLights, NumSpotLights);

					if (!Succeeded)
					{
						DEBUG_PRINT("RenderGraph failed");
						return;
					}

					for (const PrimitiveBatch* Batch : Batches)
					{
						this->RenderPrimitiveBatch(*Batch, Camera);
					}
				}
			}

			ID3D12Resource* Backbuffer = Window->GetBackCurrentBuffer();
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
			for (const std::shared_ptr<Window::RenderWindow>& Window : m_FrameWindows)
			{
				Window->Present();
			}

			m_FrameWindows.clear();

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

			// TODO: Fix dynamic resize if these dont have enough mem allocated
			// FreeListAllocator needs to be resized on demand inside ::Allocate
			/*const uint64_t MaxVertices = 1000000;
			const uint64_t MaxMeshes = 1000;
			const uint64_t MaxMaterials = 1000;*/
			// TODO: Check func and alloc size etc
			m_VertexBuffer.Init(m_Device, /*MaxVertices * */sizeof(Asset::VertexData), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_IndexBuffer.Init(m_Device,/* MaxVertices * */sizeof(uint32_t), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MeshEntryBuffer.Init(m_Device, /*MaxMeshes * */sizeof(Asset::MeshEntry), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);
			m_MaterialBuffer.Init(m_Device, /*MaxMeshes * */sizeof(Asset::MaterialData::MaterialRenderData), m_ResourceRecycler, D3D12_HEAP_TYPE_DEFAULT);

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
				m_ResourceRecycler,
				1,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				m_DSVClearColor
			);

			D3D12::Descriptor TempDescriptor;
			m_DSVHeap.GetDescriptor(TempDescriptor);
			m_SceneDepthTextureDSV = D3D12::DepthStencilView(m_Device, TempDescriptor, m_SceneDepthTexture, m_SceneDepthTexture.GetResource()->GetDesc().Format);

			m_RTVClearColor.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			m_RTVClearColor.Color[0] = 0.2f;
			m_RTVClearColor.Color[1] = 0.2f;
			m_RTVClearColor.Color[2] = 0.2f;
			m_RTVClearColor.Color[3] = 1.f;

			m_FinalRenderSurface.Init(
				m_Device,
				DXM::Vector3(m_RenderResolution.x, m_RenderResolution.y, 1),
				m_RTVClearColor.Format,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				m_ResourceRecycler,
				1,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				m_RTVClearColor
			);

			m_RTVHeap.GetDescriptor(TempDescriptor);
			m_FinalRenderSurfaceRTV = D3D12::RenderTargetView(m_Device, TempDescriptor, m_FinalRenderSurface, m_FinalRenderSurface.GetResource()->GetDesc().Format);

			D3D12::RenderPass Pass;
			{
				D3D12::Shader BasePassVS;
				BasePassVS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");

				D3D12::Shader BasePassPS;
				BasePassPS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

				Pass.Init(m_Device, BasePassVS, BasePassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_D24_UNORM_S8_UINT);
			}
			m_StaticMeshPass = RenderGraphPass(std::move(Pass));
			m_StaticMeshPass.BindBuffer("VertexBuffer", D3D12::SHADER_TYPE::VS, &m_VertexBuffer.GetBuffer());
			m_StaticMeshPass.BindBuffer("IndexBuffer", D3D12::SHADER_TYPE::VS, &m_IndexBuffer.GetBuffer());
			m_StaticMeshPass.BindBuffer("MeshEntries", D3D12::SHADER_TYPE::VS, &m_MeshEntryBuffer.GetBuffer());
			m_StaticMeshPass.BindBuffer("MaterialBuffer", D3D12::SHADER_TYPE::PS, &m_MaterialBuffer.GetBuffer());
			Rendering::RenderGraphPass::RenderTargetViewBinding RtvBinding;
			RtvBinding.Resource = &m_FinalRenderSurface;
			RtvBinding.View = &m_FinalRenderSurfaceRTV;
			m_StaticMeshPass.BindRenderTarget(0, RtvBinding);

			Rendering::RenderGraphPass::DepthStencilViewBinding DsvBinding;
			DsvBinding.Resource = &m_SceneDepthTexture;
			DsvBinding.View = &m_SceneDepthTextureDSV;
			m_StaticMeshPass.BindDepthStencil(DsvBinding);
			m_StaticMeshPass.m_ShouldClearOnExecute = true;

			m_RenderGraph.AddPass(m_StaticMeshPass, -1);

			m_RenderGraph.m_ResourceHeap = &m_ResourceHeap;
			m_RenderGraph.m_SamplerHeap = &m_SamplerHeap;
		}

		void Renderer::PrepareGraphForScene(Scene::Scene& InScene)
		{
			m_StaticMeshPass.BindBuffer("InstanceBuffer", D3D12::SHADER_TYPE::VS,
				&InScene.m_RenderProxy.GetObjects<Scene::Scene::StaticMesh>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

			m_StaticMeshPass.BindBuffer("DirectionalLightBuffer", D3D12::SHADER_TYPE::PS,
				&InScene.m_RenderProxy.GetObjects<DirectionalLightData>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

			m_StaticMeshPass.BindBuffer("PointLightBuffer", D3D12::SHADER_TYPE::PS,
				&InScene.m_RenderProxy.GetObjects<PointLightData>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

			m_StaticMeshPass.BindBuffer("SpotLightBuffer", D3D12::SHADER_TYPE::PS,
				&InScene.m_RenderProxy.GetObjects<SpotLightData>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

			struct PixelShaderConstants
			{
				uint32_t SamplerIndex;
			}PSConstants;

			PSConstants.SamplerIndex = m_DefaultSamplerDescriptor.GetHeapIndex();
			m_StaticMeshPass.BindConstant("PixelShaderConstantsBuffer", D3D12::SHADER_TYPE::PS, &PSConstants, sizeof(PixelShaderConstants));
		}

		void Renderer::InitPrimitiveBatchPipeline()
		{
			m_BatchVertexBuffer.Init(m_Device, D3D12_HEAP_TYPE_UPLOAD, 2, m_ResourceRecycler);

			D3D12::Shader PassVS;
			PassVS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "PrimitiveBatch.vs.hlsl");

			D3D12::Shader PassPS;
			PassPS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "PrimitiveBatch.ps.hlsl");

			m_BatchPassDepthP.Init(m_Device, PassVS, PassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
			m_BatchPassNoDepthP.Init(m_Device, PassVS, PassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
			m_BatchPassDepthL.Init(m_Device, PassVS, PassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
			m_BatchPassNoDepthL.Init(m_Device, PassVS, PassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
			m_BatchPassDepthT.Init(m_Device, PassVS, PassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			m_BatchPassNoDepthT.Init(m_Device, PassVS, PassPS, { m_FinalRenderSurface.GetResource()->GetDesc().Format }, DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		}

		void Renderer::RecordAssetManagerCommands()
		{
			D3D12::LinearFrameAllocator& FrameAllocator = m_FrameAllocators[m_FrameIndex];
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();
			FrameAllocator.RecordAllocations(CmdList);
			//m_GraphicsQueue.ExecuteContext(FrameAllocator.GetCommandContext());
			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
		}

		void Renderer::ClearRenderSurfaces()
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			CmdList->ClearRenderTargetView(m_FinalRenderSurfaceRTV.GetDescriptorHandle(), m_RTVClearColor.Color, 0, NULL);
			CmdList->ClearDepthStencilView(
				m_SceneDepthTextureDSV.GetDescriptorHandle(),
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				m_DSVClearColor.DepthStencil.Depth, m_DSVClearColor.DepthStencil.Stencil,
				0, NULL);

			m_GraphicsCommandContext.StopRecording();
			m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
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