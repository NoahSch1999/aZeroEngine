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