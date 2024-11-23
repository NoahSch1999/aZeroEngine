#include "Renderer.h"
#include "Core/Scene/Scene.h"

aZero::Rendering::Renderer aZero::gRenderer;

namespace aZero
{
	namespace Rendering
	{
		// Public

		void Renderer::Init(ECS::ComponentManagerDecl& ComponentManager, const DXM::Vector2& RenderResolution)
		{
			m_ComponentManager = &ComponentManager;
			m_RenderResolution = RenderResolution;

			m_DescriptorManager.Initialize(gDevice.Get());
			m_TextureFileAssets.Init(m_DescriptorManager);

			m_GraphicsQueue.Init(D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_GraphicsCommandContext.Init(D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_PackedGPULookupBufferUpdateContext.Init(D3D12_COMMAND_LIST_TYPE_DIRECT);

			m_RenderScene.Init(MAX_ENTITIES, 100);

			SetupRenderPipeline();
		}

		void Renderer::BeginFrame()
		{
			m_FrameIndex = m_FrameCount % BUFFERING_COUNT;
			m_GraphicsCommandContext.StartRecording();
		}

		void Renderer::Render(D3D12::SwapChain& SwapChain)
		{
			if (m_CurrentScene)
			{
				this->PrepareGPUBuffers();
				this->RenderPrimitives();
				this->CopyRenderSurfaceToBackBuffer(SwapChain);

				m_GraphicsCommandContext.StopRecording();
				m_GraphicsQueue.ExecuteContext({ m_GraphicsCommandContext });
				this->Present(SwapChain);
			}
		}

		void Renderer::EndFrame()
		{
			if (m_FrameCount % BUFFERING_COUNT == 0/*true*/)
			{
				m_GraphicsQueue.FlushCommands();
				gResourceRecycler.Clear();
				m_GraphicsCommandContext.FreeCommandBuffer();
				m_PackedGPULookupBufferUpdateContext.FreeCommandBuffer();
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
			m_FinalRenderSurface.Init({ m_RenderResolution.x, m_RenderResolution.y, 1 }, Desc.Format, Desc.Flags, Desc.MipLevels,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, m_RTVClearColor, &gResourceRecycler);

			D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
			UavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			UavDesc.Texture2D.MipSlice = 0;
			UavDesc.Texture2D.PlaneSlice = 0;
			UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			gDevice->CreateUnorderedAccessView(m_FinalRenderSurface.GetResource(), nullptr, &UavDesc, m_FinalRenderSurfaceUAV.GetCPUHandle());

			D3D12_RENDER_TARGET_VIEW_DESC RtvDesc;
			RtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			RtvDesc.Texture2D.MipSlice = 0;
			RtvDesc.Texture2D.PlaneSlice = 0;
			RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			gDevice->CreateRenderTargetView(m_FinalRenderSurface.GetResource(), &RtvDesc, m_FinalRenderSurfaceRTV.GetCPUHandle());

			Desc = m_SceneDepthTexture.GetResource()->GetDesc();
			m_SceneDepthTexture.Init({ m_RenderResolution.x, m_RenderResolution.y, 1 }, Desc.Format, Desc.Flags, Desc.MipLevels,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, m_DSVClearColor, &gResourceRecycler);

			D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
			DsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			DsvDesc.Texture2D.MipSlice = 0;
			DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			gDevice->CreateDepthStencilView(m_SceneDepthTexture.GetResource(), &DsvDesc, m_SceneDepthTextureDSV.GetCPUHandle());
		}

		// Private

		void Renderer::SetupRenderPipeline()
		{
			m_BasicMaterialGPUBuffer.Init(10, MAX_ENTITIES);

			m_FrameAllocators.resize(BUFFERING_COUNT);
			for (int BufferIndex = 0; BufferIndex < BUFFERING_COUNT; BufferIndex++)
			{
				m_FrameAllocators[BufferIndex].Init(m_PackedGPULookupBufferUpdateContext, MAX_ENTITIES * sizeof(PrimitiveRenderData));
			}

			m_DefaultSamplerDescriptor = m_DescriptorManager.GetSamplerHeap().GetDescriptor();
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
			gDevice->CreateSampler(&SamplerDesc, m_DefaultSamplerDescriptor.GetCPUHandle());

			m_DSVClearColor.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_DSVClearColor.DepthStencil.Depth = 1.f;
			m_DSVClearColor.DepthStencil.Stencil = 0;
			m_SceneDepthTexture = std::move(
				D3D12::GPUTexture(
					DXM::Vector3(m_RenderResolution.x, m_RenderResolution.y, 1),
					DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT,
					D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
					1,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					m_DSVClearColor,
					std::nullopt)
			);

			m_SceneDepthTextureDSV = m_DescriptorManager.GetDsvHeap().GetDescriptor();

			D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
			DsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			DsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			DsvDesc.Texture2D.MipSlice = 0;
			DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			gDevice->CreateDepthStencilView(m_SceneDepthTexture.GetResource(), &DsvDesc, m_SceneDepthTextureDSV.GetCPUHandle());

			m_RTVClearColor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			m_RTVClearColor.Color[0] = 0.2f;
			m_RTVClearColor.Color[1] = 0.2f;
			m_RTVClearColor.Color[2] = 0.2f;
			m_RTVClearColor.Color[3] = 1.f;
			m_FinalRenderSurface = std::move(
				D3D12::GPUTexture(
					DXM::Vector3(m_RenderResolution.x, m_RenderResolution.y, 1),
					DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
					1,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					m_RTVClearColor,
					std::nullopt)
			);

			D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
			UavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			UavDesc.Texture2D.MipSlice = 0;
			UavDesc.Texture2D.PlaneSlice = 0;
			UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			m_FinalRenderSurfaceUAV = m_DescriptorManager.GetResourceHeap().GetDescriptor();
			gDevice->CreateUnorderedAccessView(m_FinalRenderSurface.GetResource(), nullptr, &UavDesc, m_FinalRenderSurfaceUAV.GetCPUHandle());

			D3D12_RENDER_TARGET_VIEW_DESC RtvDesc;
			RtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			RtvDesc.Texture2D.MipSlice = 0;
			RtvDesc.Texture2D.PlaneSlice = 0;
			RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			m_FinalRenderSurfaceRTV = m_DescriptorManager.GetRtvHeap().GetDescriptor();
			gDevice->CreateRenderTargetView(m_FinalRenderSurface.GetResource(), &RtvDesc, m_FinalRenderSurfaceRTV.GetCPUHandle());

			{
				D3D12::GraphicsPass::PassDescription BasePassDesc;

				m_ShaderCache.CompileAndStore("BasePassVS");
				m_ShaderCache.CompileAndStore("BasePassPS");
				D3D12::Shader* BasePassVS = m_ShaderCache.GetShader("BasePassVS");
				BasePassVS->AddParameter<D3D12::Shader::RootConstant>(D3D12::Shader::RootConstant("VertexShaderConstants", 0, 32));
				BasePassVS->AddParameter<D3D12::Shader::RootConstant>(D3D12::Shader::RootConstant("PerDrawConstants", 1, 1));
				BasePassVS->AddParameter<D3D12::Shader::RootDescriptor>(D3D12::Shader::RootDescriptor("PrimitiveRenderDataBuffer", 2, D3D12_ROOT_PARAMETER_TYPE_SRV));

				D3D12::Shader* BasePassPS = m_ShaderCache.GetShader("BasePassPS");
				BasePassPS->AddParameter<D3D12::Shader::RootConstant>(D3D12::Shader::RootConstant("PixelShaderConstants", 0, 1));
				BasePassPS->AddParameter<D3D12::Shader::RootConstant>(D3D12::Shader::RootConstant("PerDrawConstants", 1, 1));
				BasePassPS->AddParameter<D3D12::Shader::RootDescriptor>(D3D12::Shader::RootDescriptor("PrimitiveRenderDataBuffer", 2, D3D12_ROOT_PARAMETER_TYPE_SRV));
				BasePassPS->AddParameter<D3D12::Shader::RootDescriptor>(D3D12::Shader::RootDescriptor("BasicMaterialRenderDataBuffer", 3, D3D12_ROOT_PARAMETER_TYPE_SRV));
				BasePassPS->AddRenderTarget(m_FinalRenderSurface.GetResource()->GetDesc().Format);

				BasePassDesc.VertexShader = *BasePassVS;
				BasePassDesc.PixelShader = *BasePassPS;
				BasePassDesc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				BasePassDesc.BlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				BasePassDesc.DepthStencilDesc.Description = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				BasePassDesc.DepthStencilDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
				BasePassDesc.RasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				BasePassDesc.RasterDesc.CullMode = D3D12_CULL_MODE_FRONT;
				BasePassDesc.SampleDesc.Count = 1;
				BasePassDesc.SampleDesc.Quality = 0;
				InputLayout Layout;
				Layout.AddElement("POSITION", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
				Layout.AddElement("UV", DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT);
				Layout.AddElement("NORMAL", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
				Layout.AddElement("TANGENT", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
				BasePassDesc.InputLayoutDesc.NumElements = Layout.GetNumElements();
				BasePassDesc.InputLayoutDesc.pInputElementDescs = Layout.GetDescription();

				D3D12::GraphicsPass BasePass(gDevice.Get(), BasePassDesc);
				m_GraphicsPassCache.emplace("BasePass", std::move(BasePass));
			}
		}

		void Renderer::PrepareGPUBuffers()
		{
			D3D12::FrameAllocator& FrameAllocator = m_FrameAllocators[m_FrameIndex];

			m_PackedGPULookupBufferUpdateContext.StartRecording();
			FrameAllocator.RecordAllocations();
			m_PackedGPULookupBufferUpdateContext.StopRecording();

			m_GraphicsQueue.ExecuteContext({ m_PackedGPULookupBufferUpdateContext });
		}

		void Renderer::RenderPrimitives()
		{
			ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();

			D3D12::GraphicsPass& BasePass = m_GraphicsPassCache.at("BasePass");
			BasePass.BeginPass(CmdList, m_DescriptorManager.GetResourceHeap().GetDescriptorHeap(), m_DescriptorManager.GetSamplerHeap().GetDescriptorHeap());

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
				// Per Pass
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

				PixelShaderConstants PSConstants;
				PSConstants.SamplerIndex = 0;

				BasePass.SetShaderRootConstant<D3D12::ShaderTypes::VertexShader>(CmdList, "VertexShaderConstants", &VSConstants);
				BasePass.SetShaderRootConstant<D3D12::ShaderTypes::PixelShader>(CmdList, "PixelShaderConstants", &PSConstants);
				BasePass.SetShaderRootDescriptor<D3D12::ShaderTypes::VertexShader, D3D12::DescriptorTypes::SRV>(CmdList, "PrimitiveRenderDataBuffer", m_RenderScene.GetRenderBuffer<PrimitiveRenderData>()->GetGPUVirtualAddress());
				BasePass.SetShaderRootDescriptor<D3D12::ShaderTypes::PixelShader, D3D12::DescriptorTypes::SRV>(CmdList, "PrimitiveRenderDataBuffer", m_RenderScene.GetRenderBuffer<PrimitiveRenderData>()->GetGPUVirtualAddress());
				BasePass.SetShaderRootDescriptor<D3D12::ShaderTypes::PixelShader, D3D12::DescriptorTypes::SRV>(CmdList, "BasicMaterialRenderDataBuffer", m_BasicMaterialGPUBuffer.GetResource()->GetGPUVirtualAddress());
				//

				// Per draw
				for (auto const& [Name, Entity] : m_CurrentScene->GetEntityMap())
				{

					PerDrawConstants PerDrawConstants;
					PerDrawConstants.PrimitiveIndex = Entity.GetID();

					BasePass.SetShaderRootConstant<D3D12::ShaderTypes::VertexShader>(CmdList, "PerDrawConstants", &PerDrawConstants);
					BasePass.SetShaderRootConstant<D3D12::ShaderTypes::PixelShader>(CmdList, "PerDrawConstants", &PerDrawConstants);

					ECS::StaticMeshComponent& MeshComp = *m_ComponentManager->GetComponentArray<ECS::StaticMeshComponent>().GetComponent(Entity.GetID());
					const Asset::MeshAsset* Mesh = m_Meshes.GetAsset(/*MeshComp.m_AssetNameRef*/"Torus");
					BasePass.SetIndexBuffer(CmdList, Mesh->GetIndexView(), 0, 1);
					BasePass.SetVertexBuffer(CmdList, Mesh->GetVertexView(), 0, 1);
					BasePass.DrawIndexedInstanced(CmdList, Mesh->GetNumIndices(), 1, 0, 0, 0);
				}

				//
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