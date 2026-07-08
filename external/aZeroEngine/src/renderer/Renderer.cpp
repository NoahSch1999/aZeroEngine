#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "WireframeRenderer.hpp"
#include "FrameContext.hpp"
#include "render_api/SwapChain.hpp"
#include "pipeline/RenderPass.hpp"
#include "assets/FBX_Loading.hpp"
#include "assets/MeshPrimitives.hpp"

#include "WinPixEventRuntime/pix3.h"

namespace aZero
{
	namespace Rendering
	{
		void Renderer::CompilePipeline()
		{
			Pipeline::RenderPass meshCullPass;
			Pipeline::Shader meshCullCS;

			Pipeline::RenderPass meshletDrawPass;
			Pipeline::Shader meshletDrawAS;
			Pipeline::Shader meshletDrawMS;
			Pipeline::Shader meshletDrawPS;

			meshCullCS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "MeshCull.cs.hlsl");
			meshCullPass.CompileComputePass(m_diDevice, meshCullCS);

			meshletDrawAS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "MeshletDraw.as.hlsl");
			meshletDrawMS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "MeshletDraw.ms.hlsl");
			meshletDrawPS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "Default_Phong.ps.hlsl");

			Pipeline::RenderPass::Desc passDesc;
			passDesc.RtvFormats.push_back(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			passDesc.DsvFormat = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
			const bool res = meshletDrawPass.CompileMeshletPass(passDesc, m_diDevice, meshletDrawAS, meshletDrawMS, meshletDrawPS);

			if (!res) {
				return;
			}

			m_MeshCullCS = std::move(meshCullCS);
			m_MeshCullPass = std::move(meshCullPass);
			m_MeshletDrawAS = std::move(meshletDrawAS);
			m_MeshletDrawMS = std::move(meshletDrawMS);
			m_MeshletDrawPS = std::move(meshletDrawPS);
			m_MeshletDrawPass = std::move(meshletDrawPass);
		}

		void Renderer::temp_LoadVB(FBX::FBX_Mesh& mesh)
		{
			temp_vBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(Asset::Vertex) * 100000, D3D12_HEAP_TYPE_DEFAULT), &m_ResourceRecycler);
			temp_vbv.BufferLocation = temp_vBuffer.GetResource()->GetGPUVirtualAddress();
			temp_vbv.SizeInBytes = mesh.Submeshes[0].Vertices.size() * sizeof(Asset::Vertex);
			temp_vbv.StrideInBytes = sizeof(Asset::Vertex);

			temp_pBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(DXM::Vector3) * 100000, D3D12_HEAP_TYPE_DEFAULT), &m_ResourceRecycler);
			temp_pbv.BufferLocation = temp_pBuffer.GetResource()->GetGPUVirtualAddress();
			temp_pbv.StrideInBytes = sizeof(DXM::Vector3);

			temp_iBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(Asset::Index) * 100000, D3D12_HEAP_TYPE_DEFAULT), &m_ResourceRecycler);
			temp_ibv.BufferLocation = temp_iBuffer.GetResource()->GetGPUVirtualAddress();
			temp_ibv.SizeInBytes = sizeof(Asset::Index) * mesh.Submeshes[0].Indices.size();
			temp_ibv.Format = DXGI_FORMAT_R32_UINT;

			Pipeline::Shader vs;
			vs.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "DebugLine.vs.hlsl");
			Pipeline::Shader ps;
			ps.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "DebugLine.ps.hlsl");

			Pipeline::RenderPass::VertexPassDesc vsDesc;
			vsDesc.DsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			vsDesc.RtvFormats.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
			vsDesc.TopologyType = Pipeline::ETopologyType::LINE;
			//m_Pass.CompileVertexPass(vsDesc, m_diDevice, vs, ps);
		}

		Renderer::Renderer(ID3D12DeviceX* device, uint32_t bufferCount, IDxcCompilerX& compiler)
			:m_diCompiler(compiler), m_diDevice(device)
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
			device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData));

			if (featureData.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
				throw std::runtime_error("Device doesn't support mesh shaders.");
			}

			m_ResourceRecycler = RenderAPI::ResourceRecycler(bufferCount);

			m_DirectCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_CopyCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY);
			m_ComputeCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

			m_ResourceHeap = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, true);
			m_SamplerHeap = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeap = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeap = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);

#ifdef USE_DEBUG
			m_DirectCommandQueue.Get()->SetName(L"m_DirectCommandQueue");
			m_CopyCommandQueue.Get()->SetName(L"m_CopyCommandQueue");
			m_ComputeCommandQueue.Get()->SetName(L"m_ComputeCommandQueue");
			m_ResourceHeap.Get()->SetName(L"m_ResourceHeap");
			m_SamplerHeap.Get()->SetName(L"m_SamplerHeap");
			m_RTVHeap.Get()->SetName(L"m_RTVHeap");
			m_DSVHeap.Get()->SetName(L"m_DSVHeap");
#endif

			// TODO: DEFINE A MOVE-CONSTRUCTOR FOR FRAMECONTEXT
			// OTHERWISE THIS WILL CRASH IF WE DONT RESERVE
			m_FrameContexts.reserve(bufferCount);
			for (int32_t i = 0; i < bufferCount; i++)
			{
				m_FrameContexts.emplace_back(device, m_ResourceHeap, m_ResourceRecycler, MAX_INSTANCES);
			}
			 
			m_SamplerManager = SamplerManager(device, m_SamplerHeap);

			m_RenderAssetManager = std::make_unique<Rendering::RenderAssetManager>(device, m_ResourceRecycler, m_ResourceHeap);

			m_WireframeRenderer = std::make_unique<Rendering::WireframeRenderer>(*this, device, compiler);

			this->InitGPUDrivenRenderPipeline();
		}

		bool Renderer::AdvanceFrameIfReady()
		{
			const uint32_t newPotentialFrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());

			if (!m_FrameContexts.at(newPotentialFrameIndex).IsReady(m_DirectCommandQueue))
			{
				return false;
			}

			m_FrameContexts.at(newPotentialFrameIndex).Begin();

			return true;
		}

		bool Renderer::TryBeginFrame()
		{
			const bool hasNewFrameStarted = AdvanceFrameIfReady();
			if (hasNewFrameStarted)
			{
				m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());
				m_FrameCount++;
				m_ResourceRecycler.SetFrameIndex(m_FrameIndex);
				m_ResourceRecycler.Clear();
				m_WireframeRenderer->BeginFrame();
			}

			return hasNewFrameStarted;
		}

		void Renderer::EndFrame()
		{
			FrameContext& frameContext = this->GetCurrentContext();
			frameContext.SetLatestSignal(m_DirectCommandQueue.Signal());
		}

		void Renderer::FlushFrameAllocations()
		{
			FrameContext& frameContext = this->GetCurrentContext();
			frameContext.RecordFrameAllocations(frameContext.GetCommandList());
			m_DirectCommandQueue.ExecuteCommandList(frameContext.GetCommandList(), true);
		}

		void Renderer::InitGPUDrivenRenderPipeline()
		{
			this->CompilePipeline();

			m_IndirectArgumentCounter = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::IndirectArgumentCounter) * 1, D3D12_HEAP_TYPE_DEFAULT, true));
			m_IndirectArguments = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::IndirectArguments) * MAX_INSTANCES, D3D12_HEAP_TYPE_DEFAULT, true));

			std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2> iaArgs{};
			iaArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			iaArgs[0].Constant.RootParameterIndex = m_MeshletDrawPass.GetConstantBinding("Input_CONSTANT").value().get().GetRootIndex();
			iaArgs[0].Constant.Num32BitValuesToSet = m_MeshletDrawPass.GetConstantBinding("Input_CONSTANT").value().get().GetNumConstants();
			iaArgs[0].Constant.DestOffsetIn32BitValues = 0;
			iaArgs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			D3D12_COMMAND_SIGNATURE_DESC iaArgsDesc{};
			iaArgsDesc.pArgumentDescs = iaArgs.data();
			iaArgsDesc.NumArgumentDescs = iaArgs.size();
			iaArgsDesc.ByteStride = sizeof(GPU_Struct::IndirectArguments);
			m_diDevice->CreateCommandSignature(&iaArgsDesc, m_MeshletDrawPass.GetRootSignature(), IID_PPV_ARGS(m_MeshletDrawSignature.GetAddressOf()));

#ifdef USE_DEBUG
			m_IndirectArgumentCounter.GetResource()->SetName(L"m_IndirectArgumentCounter");
			m_IndirectArguments.GetResource()->SetName(L"m_IndirectArguments");
			m_MeshletDrawSignature->SetName(L"m_MeshletDrawSignature");
			m_MeshCullPass.GetPipelineState()->SetName(L"m_MeshCullPass");
			m_MeshletDrawPass.GetPipelineState()->SetName(L"m_MeshletDrawPass");
#endif
		}

		void Renderer::RecordGPUDrivenRenderPipeline(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, Scene::Scene& scene)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			RenderAPI::CommandList& cmdList = frameContext.GetCommandList();
			std::array<D3D12_RESOURCE_BARRIER, 2> barriers;

			auto [renderDataFrameInfo, renderData] = scene.GetRenderData(frameContext.GetFrameUploadAllocator(), frameContext.GetFrameUploadBuffer(), cmdList);

			this->ClearRenderTarget(renderTarget);
			this->ClearDepthStencilTarget(depthStencilTarget);

			// TODO: Use the scene camera buffer and render for each camera
			D3D12_VIEWPORT viewport = renderData.get().CameraRSData[0];
			D3D12_RECT scizzorRect{
				.left = (LONG)viewport.TopLeftX,
				.top = (LONG)viewport.TopLeftY,
				.right = (LONG)viewport.TopLeftX + (LONG)viewport.Width,
				.bottom = (LONG)viewport.TopLeftY + (LONG)viewport.Height
			};

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "Frame setup");

				// Pre-pass setup
				cmdList.SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

				barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArgumentCounter.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
				cmdList->ResourceBarrier(1, barriers.data());

				// Reset the counting buffer used in the MeshCull pass
				uint32_t count = 0;
				frameContext.AddAllocation(count, m_IndirectArgumentCounter, 0);
				frameContext.GetFrameStagingAllocator().RecordAllocations(cmdList);
			}

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "Mesh culling pass");

				barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArguments.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArgumentCounter.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				cmdList->ResourceBarrier(2, barriers.data());

				auto meshCullConstants = m_MeshCullPass.GetConstantBinding("MeshCull_CONSTANT");
				auto cameraBuffer = m_MeshCullPass.GetBufferBinding("CameraDataBuffer");
				auto objectCullDataBuffer = m_MeshCullPass.GetBufferBinding("ObjectCullDataBuffer");
				auto indirectArgCounter = m_MeshCullPass.GetBufferBinding("IndirectArgumentCounterBuffer");
				auto indirectArgBuffer = m_MeshCullPass.GetBufferBinding("IndirectArgumentsBuffer");
				auto instanceDataBufferMS = m_MeshCullPass.GetBufferBinding("InstanceDataBufferMS");

				cmdList.SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

				m_MeshCullPass.Begin(cmdList);

				aZero::Rendering::GPU_Struct::MeshCullConstantsData meshletCullConstantsData;
				meshletCullConstantsData.MeshInstanceCount = renderDataFrameInfo.MeshCount;
				cmdList.SetComputeRoot32BitConstantsSafe(meshCullConstants, &meshletCullConstantsData, 0);
				cmdList.SetComputeRootUnorderedAccessViewSafe(indirectArgCounter, m_IndirectArgumentCounter.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootUnorderedAccessViewSafe(indirectArgBuffer, m_IndirectArguments.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeConstantBufferViewSafe(cameraBuffer, renderData.get().CameraBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootShaderResourceViewSafe(objectCullDataBuffer, renderData.get().ObjectCullDataBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootShaderResourceViewSafe(instanceDataBufferMS, renderData.get().InstanceBuffer.GetResource()->GetGPUVirtualAddress());

				cmdList->Dispatch(std::ceil(renderDataFrameInfo.MeshCount / 32.f), 1, 1);
			}

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "Meshlet culling and drawing pass");

				barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_IndirectArguments.GetResource());
				barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_IndirectArgumentCounter.GetResource());
				cmdList->ResourceBarrier(2, barriers.data());

				barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArguments.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArgumentCounter.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				cmdList->ResourceBarrier(2, barriers.data());

				auto cameraBuffer = m_MeshletDrawPass.GetBufferBinding("CameraDataBuffer");
				auto instanceDataBufferAS = m_MeshletDrawPass.GetBufferBinding("InstanceDataBufferAS");
				auto meshletBoundBuffer = m_MeshletDrawPass.GetBufferBinding("MeshletBoundBuffer");

				auto instanceDataBufferMS = m_MeshletDrawPass.GetBufferBinding("InstanceDataBufferMS");
				auto meshletBuffer = m_MeshletDrawPass.GetBufferBinding("MeshletBuffer");
				auto vertexBuffer = m_MeshletDrawPass.GetBufferBinding("VertexBuffer");

				auto materialBuffer = m_MeshletDrawPass.GetBufferBinding("MaterialBuffer");

				m_MeshletDrawPass.Begin(cmdList);
				cmdList.OMSetRenderTargets({ renderTarget.GetDescriptor() }, depthStencilTarget.GetDescriptor());

				cmdList.SetGraphicsConstantBufferViewSafe(cameraBuffer, renderData.get().CameraBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetGraphicsRootShaderResourceViewSafe(instanceDataBufferAS, renderData.get().InstanceBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetGraphicsRootShaderResourceViewSafe(meshletBoundBuffer, m_RenderAssetManager.get()->m_MeshletBoundsBuffer.GetResource()->GetGPUVirtualAddress());

				cmdList.SetGraphicsRootShaderResourceViewSafe(instanceDataBufferMS, renderData.get().InstanceBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetGraphicsRootShaderResourceViewSafe(meshletBuffer, m_RenderAssetManager.get()->m_MeshletBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetGraphicsRootShaderResourceViewSafe(vertexBuffer, m_RenderAssetManager.get()->m_VertexBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetGraphicsRootShaderResourceViewSafe(materialBuffer, m_RenderAssetManager.get()->m_MaterialDataBuffer.GetBuffer().GetResource()->GetGPUVirtualAddress());

				cmdList->RSSetScissorRects(1, &scizzorRect);
				cmdList->RSSetViewports(1, &viewport);

				cmdList->ExecuteIndirect(m_MeshletDrawSignature.Get(), MAX_INSTANCES, m_IndirectArguments.GetResource(), 0, m_IndirectArgumentCounter.GetResource(), 0);
			}

			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);
		}

		void Renderer::Render(Scene::Scene& scene, Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.GetCommandList();

			PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 0, 0), "Render scene");

			frameContext.RecordFrameAllocations(cmdList);

			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);

			this->RecordGPUDrivenRenderPipeline(renderTarget, depthStencilTarget, scene);
		}

		void Renderer::FlushRenderCommands()
		{
			m_DirectCommandQueue.Flush();
		}

		void Renderer::CopyRenderTargetToSwapChain(RenderAPI::SwapChain& swapChain, Rendering::RenderTarget& renderTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.GetCommandList();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, swapChain.GetFrameBackBuffer() });
			preCopyBarriers.push_back({ renderTarget.GetTexture().GetState(), D3D12_RESOURCE_STATE_COPY_SOURCE, renderTarget.GetTexture().GetResource() });

			RenderAPI::TransitionResources(cmdList, preCopyBarriers);

			// TODO: Handle up/down-scaling when missmatched resources
			cmdList->CopyResource(swapChain.GetFrameBackBuffer(), renderTarget.GetTexture().GetResource());

			std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, swapChain.GetFrameBackBuffer() });
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, renderTarget.GetTexture().GetResource() });
			renderTarget.GetTexture().CreateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET); // To update the internal state

			RenderAPI::TransitionResources(cmdList, postCopyBarriers);

			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);
		}

		void  Renderer::ClearRenderTarget(Rendering::RenderTarget& rtv)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.GetCommandList();

			if (rtv.GetTexture().GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				auto barrier = rtv.GetTexture().CreateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET);
				cmdList->ResourceBarrier(1, &barrier);
			}
			cmdList->ClearRenderTargetView(rtv.GetCpuHandle(), rtv.GetClearValue().Color, 0, nullptr);
		}

		void Renderer::ClearDepthStencilTarget(Rendering::DepthStencilTarget& dsv)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.GetCommandList();
			if (dsv.GetTexture().GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
			{
				auto barrier = dsv.GetTexture().CreateTransition(D3D12_RESOURCE_STATE_DEPTH_WRITE);
				cmdList->ResourceBarrier(1, &barrier);
			}
			const auto value = dsv.GetClearValue().DepthStencil;
			cmdList->ClearDepthStencilView(dsv.GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, value.Depth, value.Stencil, 0, nullptr);
		}

		Rendering::RenderTarget Renderer::CreateRenderTarget(const Rendering::RenderTarget::Desc& desc)
		{
			return Rendering::RenderTarget(desc, m_diDevice, m_RTVHeap, &m_ResourceRecycler);
		}

		Rendering::DepthStencilTarget Renderer::CreateDepthStencilTarget(const Rendering::DepthStencilTarget::Desc& desc)
		{
			return Rendering::DepthStencilTarget(desc, m_diDevice, m_DSVHeap, &m_ResourceRecycler);
		}

		Rendering::WireframeRenderer& Renderer::GetWireframeRenderer() { return *m_WireframeRenderer.get(); }
	}
}