#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "WireframeRenderer.hpp"
#include "graphics_api/resource/buffer/MeshBuffer.hpp"
#include "FrameContext.hpp"
#include "graphics_api/SwapChain.hpp"
#include "assets/Asset.hpp"
#include "pipeline/RenderPass.hpp"

#include "WinPixEventRuntime/pix3.h"

namespace aZero
{
	namespace Rendering
	{
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

			m_ResourceManager = ResourceManager(device, &m_ResourceRecycler, m_ResourceHeap);

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

			// Perform uploads for all updated/new assets and other stagings
			frameContext.RecordFrameAllocations(frameContext.m_DirectCmdList);
			frameContext.SetLatestSignal(m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, true));
			frameContext.m_FrameAllocator.ClearQueuedAllocations();
		}


		void Renderer::InitGPUDrivenRenderPipeline()
		{
			m_MeshCullCS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "MeshCull.cs.hlsl");
			m_MeshCullPass.CompileComputePass(m_diDevice, m_MeshCullCS);

			m_MeshletDrawAS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "MeshletDraw.as.hlsl");
			m_MeshletDrawMS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "MeshletDraw.ms.hlsl");
			m_MeshletDrawPS.Compile(m_diCompiler, Pipeline::GetShaderDirectoryPath() + "Default_Phong.ps.hlsl");

			Pipeline::RenderPass::Desc passDesc;
			passDesc.RtvFormats.push_back(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			passDesc.DsvFormat = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_MeshletDrawPass.CompileMeshletPass(passDesc, m_diDevice, m_MeshletDrawAS, m_MeshletDrawMS, m_MeshletDrawPS);

			m_IndirectArgumentCounter = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::IndirectArgumentCounter) * 1, D3D12_HEAP_TYPE_DEFAULT, true));
			m_IndirectArguments = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(GPU_Struct::IndirectArguments) * MAX_INSTANCES, D3D12_HEAP_TYPE_DEFAULT, true));

			std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2> iaArgs;
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

		void Renderer::RecordGPUDrivenRenderPipeline(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, const Rendering::GPUProxy::Camera& camera, uint32_t numStaticMeshes)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			RenderAPI::CommandList& cmdList = frameContext.m_DirectCmdList;
			std::array<D3D12_RESOURCE_BARRIER, 2> barriers;

			GPU_Struct::CameraData cameraStruct;
			cameraStruct.Frustum = camera.m_Frustrum;
			cameraStruct.ViewMatrix = camera.m_View;
			cameraStruct.ViewProjectionMatrix = camera.m_View * camera.m_Projection;
			frameContext.m_CameraBuffer.Write(&cameraStruct, sizeof(cameraStruct), 0);

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "Frame setup");

				// Pre-pass setup
				cmdList.SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

				barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArgumentCounter.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
				cmdList->ResourceBarrier(1, barriers.data());

				// Reset the counting buffer used in the MeshCull pass
				uint32_t count = 0;
				frameContext.AddAllocation(count, m_IndirectArgumentCounter, 0);
				frameContext.m_FrameAllocator.RecordAllocations(cmdList);
			}

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "Mesh culling pass");

				barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArguments.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArgumentCounter.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				cmdList->ResourceBarrier(2, barriers.data());

				auto meshCullConstants = m_MeshCullPass.GetConstantBinding("MeshCull_CONSTANT");
				auto meshInstancesBuffer = m_MeshCullPass.GetBufferBinding("MeshInstances");
				auto indirectArgCounter = m_MeshCullPass.GetBufferBinding("IndirectArgumentCounter");
				auto indirectArgBuffer = m_MeshCullPass.GetBufferBinding("IndirectArguments");
				auto cameraBuffer = m_MeshCullPass.GetBufferBinding("CameraBuffer");

				cmdList.SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

				m_MeshCullPass.Begin(cmdList);

				aZero::Rendering::GPU_Struct::MeshCullConstantsData meshletCullConstantsData;
				meshletCullConstantsData.MeshInstanceCount = numStaticMeshes;
				cmdList.SetComputeRoot32BitConstantsSafe(meshCullConstants, &meshletCullConstantsData, 0);
				cmdList.SetComputeRootShaderResourceViewSafe(meshInstancesBuffer, frameContext.m_StaticMeshBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootUnorderedAccessViewSafe(indirectArgCounter, m_IndirectArgumentCounter.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootUnorderedAccessViewSafe(indirectArgBuffer, m_IndirectArguments.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeConstantBufferViewSafe(cameraBuffer, frameContext.m_CameraBuffer.GetResource()->GetGPUVirtualAddress());

				cmdList->Dispatch(std::ceil(numStaticMeshes / 32.f), 1, 1);
			}

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "Meshlet culling and drawing pass");

				barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_IndirectArguments.GetResource());
				barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_IndirectArgumentCounter.GetResource());
				cmdList->ResourceBarrier(2, barriers.data());

				barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArguments.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectArgumentCounter.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				cmdList->ResourceBarrier(2, barriers.data());

				auto meshletCullConstants = m_MeshletDrawPass.GetConstantBinding("MeshletCull_CONSTANT");
				auto meshInstancesBuffer = m_MeshletDrawPass.GetBufferBinding("MeshInstances");
				auto cameraBuffer = m_MeshletDrawPass.GetBufferBinding("CameraBuffer");

				m_MeshletDrawPass.Begin(cmdList);
				cmdList.OMSetRenderTargets({ renderTarget.GetDescriptor() }, depthStencilTarget.GetDescriptor());

				cmdList.SetGraphicsRootShaderResourceViewSafe(meshInstancesBuffer, frameContext.m_StaticMeshBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetGraphicsConstantBufferViewSafe(cameraBuffer, frameContext.m_CameraBuffer.GetResource()->GetGPUVirtualAddress());

				cmdList->RSSetScissorRects(1, &camera.m_RSInfo.ScizzorRect);
				cmdList->RSSetViewports(1, &camera.m_RSInfo.Viewport);

				cmdList->ExecuteIndirect(m_MeshletDrawSignature.Get(), MAX_INSTANCES, m_IndirectArguments.GetResource(), 0, m_IndirectArgumentCounter.GetResource(), 0);
			}

			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);
		}

		void Renderer::Render(const Scene::Scene& scene, Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.m_DirectCmdList;

			PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 0, 0), "Render scene");

			frameContext.RecordFrameAllocations(cmdList);

			auto [staticMeshes, cameras] = scene.GetWorldRenderData();

			this->ClearRenderTarget(renderTarget);
			this->ClearDepthStencilTarget(depthStencilTarget);

			//if (staticMeshes.size() == 0 || cameras.size() == 0) { return; }

			frameContext.m_StaticMeshBuffer.Write(staticMeshes.data(), staticMeshes.size() * sizeof(staticMeshes[0]), 0);

			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);

			// TODO: Change so not only the camera at index[0] will be used
			// TODO: Render with each camera and its dsv/rtv or allow many ::Render() calls in a frame for the same scene...
			this->RecordGPUDrivenRenderPipeline(renderTarget, depthStencilTarget, cameras[0], staticMeshes.size());
		}

		void Renderer::FlushRenderCommands()
		{
			m_DirectCommandQueue.Flush();

			// todo When we're also using other types of queues we need to add them here and do some other stuff
		}

		void Renderer::CopyRenderTargetToSwapChain(RenderAPI::SwapChain& swapChain, Rendering::RenderTarget& renderTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, swapChain.GetFrameBackBuffer() });
			preCopyBarriers.push_back({ renderTarget.GetTexture().GetState(), D3D12_RESOURCE_STATE_COPY_SOURCE, renderTarget.GetTexture().GetResource() });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, preCopyBarriers);

			// TODO: Handle up/down-scaling when missmatched resources
			frameContext.m_DirectCmdList->CopyResource(swapChain.GetFrameBackBuffer(), renderTarget.GetTexture().GetResource());

			std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, swapChain.GetFrameBackBuffer() });
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, renderTarget.GetTexture().GetResource() });
			renderTarget.GetTexture().CreateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET); // To update the internal state

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, postCopyBarriers);

			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);
		}

		void Renderer::UpdateRenderState(Asset::Mesh& mesh)
		{
			FrameContext& context = this->GetCurrentContext();
			m_ResourceManager.UpdateRenderState(m_diDevice, context.m_DirectCmdList, context.m_FrameAllocator, m_ResourceRecycler, m_ResourceHeap, mesh);
			m_DirectCommandQueue.ExecuteCommandList(context.m_DirectCmdList);
		}

		void Renderer::UpdateRenderState(Asset::Material& material)
		{
			m_ResourceManager.UpdateRenderState(this->GetCurrentContext().m_FrameAllocator, m_ResourceRecycler, m_ResourceHeap, material);
		}

		void Renderer::UpdateRenderState(Asset::Texture& texture)
		{
			FrameContext& context = this->GetCurrentContext();
			m_ResourceManager.UpdateRenderState(m_diDevice, context.m_DirectCmdList, m_ResourceRecycler, m_ResourceHeap, texture);
			m_DirectCommandQueue.ExecuteCommandList(context.m_DirectCmdList);
		}

		void Renderer::RemoveRenderState(Asset::Mesh& mesh)
		{
			//m_ResourceManager.
		}

		void Renderer::RemoveRenderState(Asset::Material& material)
		{
			//m_ResourceManager.
		}

		void Renderer::RemoveRenderState(Asset::Texture& texture)
		{
			//m_ResourceManager.
		}

		void  Renderer::ClearRenderTarget(Rendering::RenderTarget& rtv)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.m_DirectCmdList;

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
			auto& cmdList = frameContext.m_DirectCmdList;
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