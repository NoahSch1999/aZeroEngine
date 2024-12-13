#include "Renderer.h"
#include "Core/Scene/Scene.h"

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
					DEBUG_PRINT("Rendering::SetRenderPass() => Input pass has an invalid topology type");
				}
				}
			}
		}

		void Renderer::Init(ID3D12Device* Device, const DXM::Vector2& RenderResolution)
		{
			m_RenderResolution = RenderResolution;

			m_ResourceHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);
			m_SamplerHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeap.Init(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);

			m_GraphicsQueue.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_GraphicsCommandContext.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_PackedGPULookupBufferUpdateContext.Init(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));

			SetupRenderPipeline();
		}

		void Renderer::BeginFrame()
		{
			m_FrameIndex = m_FrameCount % BUFFERING_COUNT;
			this->GetFrameAllocator().GetCommandContext().StartRecording();
		}

		void Renderer::Render(D3D12::SwapChain& SwapChain)
		{
			if (m_CurrentScene)
			{
				this->PrepareGPUBuffers();
				this->RenderPrimitives();
				this->CopyRenderSurfaceToBackBuffer(SwapChain);

				m_GraphicsCommandContext.StopRecording();
				m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
				this->Present(SwapChain);
			}
		}

		void Renderer::EndFrame()
		{
			if (m_FrameCount % BUFFERING_COUNT == 0)
			{
				m_GraphicsQueue.FlushCommands();
				m_ResourceRecycler.ReleaseResources();
				m_GraphicsCommandContext.FreeCommandBuffer();
				m_PackedGPULookupBufferUpdateContext.FreeCommandBuffer();

				m_FrameAllocators.at(0).GetCommandContext().FreeCommandBuffer();
				m_FrameAllocators.at(1).GetCommandContext().FreeCommandBuffer();
				m_FrameAllocators.at(2).GetCommandContext().FreeCommandBuffer();
			}

			m_FrameAllocators[m_FrameIndex].Reset();
			m_FrameCount++;
		}

		void Renderer::ChangeRenderResolution(const DXM::Vector2& NewRenderResolution)
		{
			m_RenderResolution = NewRenderResolution;
			m_FrameIndex = 0;
			m_FrameCount = 0;

			D3D12_RESOURCE_DESC Desc = m_FinalRenderSurface.GetResource()->GetDesc();

			m_FinalRenderSurface.Init(
				m_Device, 
				{ m_RenderResolution.x, m_RenderResolution.y, 1 }, 
				Desc.Format, Desc.Flags, Desc.MipLevels, 
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				m_RTVClearColor,
				&m_ResourceRecycler);

			D3D12_RENDER_TARGET_VIEW_DESC RtvDesc;
			RtvDesc.Format = Desc.Format;
			RtvDesc.Texture2D.MipSlice = 0;
			RtvDesc.Texture2D.PlaneSlice = 0;
			RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			m_Device->CreateRenderTargetView(m_FinalRenderSurface.GetResource(), &RtvDesc, m_FinalRenderSurfaceRTV.GetCPUHandle());

			Desc = m_SceneDepthTexture.GetResource()->GetDesc();

			m_SceneDepthTexture.Init(
				m_Device,
				{ m_RenderResolution.x, m_RenderResolution.y, 1 },
				Desc.Format, Desc.Flags, Desc.MipLevels,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				m_DSVClearColor,
				&m_ResourceRecycler);

			D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
			DsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			DsvDesc.Texture2D.MipSlice = 0;
			DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			m_Device->CreateDepthStencilView(m_SceneDepthTexture.GetResource(), &DsvDesc, m_SceneDepthTextureDSV.GetCPUHandle());
		}

		void Renderer::SetupRenderPipeline()
		{

			m_FrameAllocators.resize(BUFFERING_COUNT);
			for (int BufferIndex = 0; BufferIndex < BUFFERING_COUNT; BufferIndex++)
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
			m_MeshEntryBuffer.Init(m_Device, MaxMeshes * sizeof(Asset::MeshGPUEntry), &m_ResourceRecycler, D3D12::GPUResource::GPUONLY);
			m_MaterialBuffer.Init(m_Device, MaxMeshes * sizeof(Asset::MaterialRenderData), &m_ResourceRecycler, D3D12::GPUResource::GPUONLY);

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
				std::vector<D3D12::Shader::RootConstantOverride> CBOverrides;
				CBOverrides.push_back({ 0 });
				CBOverrides.push_back({ 1 });
				BasePassVS.CompileFromFile(this->GetCompiler(), SHADER_SRC_PATH("BasePass.vs"), CBOverrides, {});

				D3D12::Shader BasePassPS;
				CBOverrides.clear();
				CBOverrides.push_back({ 0 });
				CBOverrides.push_back({ 1 });

				std::vector<D3D12::Shader::RenderTargetOverride> RTOverrides;
				RTOverrides.push_back({ 0, m_FinalRenderSurface.GetResource()->GetDesc().Format });
				BasePassPS.CompileFromFile(this->GetCompiler(), SHADER_SRC_PATH("BasePass.ps"), CBOverrides, RTOverrides);

				Pass.Init(m_Device, BasePassVS, BasePassPS, DXGI_FORMAT_D24_UNORM_S8_UINT);

				D3D12::Shader TestCS;
				bool CSRes = TestCS.CompileFromFile(this->GetCompiler(), SHADER_SRC_PATH("TestShader.cs"));
				
				D3D12::RenderPass TestCSPass;
				TestCSPass.Init(m_Device, TestCS);
				int x = 02;
			}
		}

		void Renderer::PrepareGPUBuffers()
		{
			D3D12::LinearFrameAllocator& FrameAllocator = m_FrameAllocators[m_FrameIndex];

			FrameAllocator.RecordAllocations();
			m_GraphicsQueue.ExecuteContext(FrameAllocator.GetCommandContext());
		}

		void Renderer::RenderPrimitives()
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap.GetDescriptorHeap(), m_SamplerHeap.GetDescriptorHeap() };
			CmdList->SetDescriptorHeaps(2, Heaps);

			SetRenderPass(CmdList, Pass);

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles = { m_FinalRenderSurfaceRTV.GetCPUHandle() };
			D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = m_SceneDepthTextureDSV.GetCPUHandle();
			CmdList->OMSetRenderTargets(RTVHandles.size(), RTVHandles.data(), false, &DSVHandle);
			CmdList->ClearRenderTargetView(m_FinalRenderSurfaceRTV.GetCPUHandle(), m_RTVClearColor.Color, 0, NULL);
			CmdList->ClearDepthStencilView(
				m_SceneDepthTextureDSV.GetCPUHandle(),
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				m_DSVClearColor.DepthStencil.Depth, m_DSVClearColor.DepthStencil.Stencil,
				0, NULL);

			D3D12_VIEWPORT Viewport;
			Viewport.TopLeftX = 0;
			Viewport.TopLeftY = 0;
			Viewport.Width = m_RenderResolution.x;
			Viewport.Height = m_RenderResolution.y;
			Viewport.MinDepth = 0;
			Viewport.MaxDepth = 1;
			CmdList->RSSetViewports(1, &Viewport);

			D3D12_RECT ScizzorRect;
			ScizzorRect.left = 0;
			ScizzorRect.top = 0;
			ScizzorRect.right = m_RenderResolution.x;
			ScizzorRect.bottom = m_RenderResolution.y;
			CmdList->RSSetScissorRects(1, &ScizzorRect);

			{
				struct VertexShaderConstants
				{
					DXM::Matrix ViewMatrix;
					DXM::Matrix ProjectionMatrix;
				};

				struct PixelShaderConstants
				{
					int SamplerIndex;
				};

				VertexShaderConstants VSConstants;
				VSConstants.ViewMatrix = DXM::Matrix::CreateLookAt({ 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f });
				VSConstants.ProjectionMatrix = DXM::Matrix::CreatePerspectiveFieldOfView(3.14f * 0.4f, m_RenderResolution.x / m_RenderResolution.y, 0.001f, 1000.f);

				Pass.SetShaderResource(CmdList, "ViewMatricesBuffer", &VSConstants, sizeof(VertexShaderConstants), D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "PrimitiveRenderDataBuffer", 
					m_CurrentScene->GetRenderScene()->GetRenderBuffer<PrimitiveRenderData>()->GetGPUVirtualAddress(), 
					D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "VertexBuffer",
					m_VertexBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "IndexBuffer",
					m_IndexBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "MeshEntries",
					m_MeshEntryBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::VS);

				Pass.SetShaderResource(CmdList, "PrimitiveRenderDataBuffer",
					m_CurrentScene->GetRenderScene()->GetRenderBuffer<PrimitiveRenderData>()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::PS
				);

				Pass.SetShaderResource(CmdList, "MaterialBuffer",
					m_MaterialBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress(),
					D3D12::SHADER_TYPE::PS
				);

				PixelShaderConstants PSConstants;
				PSConstants.SamplerIndex = 0;
				//Pass.SetShaderResource(CmdList, "PixelShaderConstantsBuffer", &PSConstants, sizeof(PixelShaderConstants), D3D12::SHADER_TYPE::PS);
				
				for (auto const& [Name, Entity] : m_CurrentScene->GetEntityMap())
				{
					PerDrawConstants PerDrawConstants;
					PerDrawConstants.PrimitiveIndex = Entity.GetEntity().GetID();
					Pass.SetShaderResource(CmdList, "PerDrawConstantsBuffer", &PerDrawConstants, sizeof(PerDrawConstants), D3D12::SHADER_TYPE::VS);
					Pass.SetShaderResource(CmdList, "PerDrawConstantsBuffer", &PerDrawConstants, sizeof(PerDrawConstants), D3D12::SHADER_TYPE::PS);

					CmdList->DrawInstanced(m_Entity_To_Mesh.at(Entity.GetEntity().GetID())->GetAssetData().Indices.size(), 1, 0, 0);
				}
			}
		}

		void Renderer::CopyRenderSurfaceToBackBuffer(D3D12::SwapChain& SwapChain)
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			std::vector<D3D12::ResourceTransitionBundles> PreCopyBarriers;
			PreCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, SwapChain.GetBackBufferResource(m_FrameIndex) });
			PreCopyBarriers.push_back({ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE, m_FinalRenderSurface.GetResource() });
			D3D12::TransitionResources(CmdList, PreCopyBarriers);

			CmdList->CopyResource(SwapChain.GetBackBufferResource(m_FrameIndex), m_FinalRenderSurface.GetResource());

			std::vector<D3D12::ResourceTransitionBundles> PostCopyBarriers;
			PostCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, SwapChain.GetBackBufferResource(m_FrameIndex) });
			PostCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, m_FinalRenderSurface.GetResource() });
			D3D12::TransitionResources(CmdList, PostCopyBarriers);
		}
	}
}