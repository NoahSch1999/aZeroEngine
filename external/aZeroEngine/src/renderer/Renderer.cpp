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

		void Renderer::InitGPUDrivenRenderPipeline()
		{
			// Mesh cull pass
			m_MeshCullCS.CompileFromFile(m_diCompiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshCull.cs.hlsl"));
			Pipeline::ComputeShaderPass::Description meshCullPassDesc;
			m_MeshCullPass.Compile(m_diDevice, meshCullPassDesc, m_MeshCullCS);
			m_MeshCull_Count_B = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(MeshCull_Count) * 1, D3D12_HEAP_TYPE_DEFAULT, true));

			// Meshlet draw pass
			m_MeshletDrawAS.CompileFromFile(m_diCompiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletDraw.as.hlsl"));
			m_MeshletDrawMS.CompileFromFile(m_diCompiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletDraw.ms.hlsl"));
			m_MeshletDrawPS.CompileFromFile(m_diCompiler, PROJECT_DIRECTORY + std::string("shaderSource/Default_Phong.ps.hlsl"));

			Pipeline::MeshShaderPass::Description meshletDrawPassDesc;
			meshletDrawPassDesc.m_RenderTargets.push_back({ DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "ColorTarget" });
			meshletDrawPassDesc.m_DepthStencil.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_MeshletDrawPass.Compile(m_diDevice, meshletDrawPassDesc, &m_MeshletDrawAS, m_MeshletDrawMS, &m_MeshletDrawPS);

			// Indirect arguments written to in the MeshCull pass
			std::array<D3D12_INDIRECT_ARGUMENT_DESC, 3> meshCullIA;
			meshCullIA[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			meshCullIA[0].Constant.RootParameterIndex = m_MeshletDrawPass.GetConstantBindingIndex("MeshletDrawConstants").GetRootIndex();
			meshCullIA[0].Constant.Num32BitValuesToSet = m_MeshletDrawPass.GetConstantBindingIndex("MeshletDrawConstants").GetNumConstants();
			meshCullIA[0].Constant.DestOffsetIn32BitValues = 0;
			meshCullIA[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			meshCullIA[1].Constant.RootParameterIndex = m_MeshletDrawPass.GetConstantBindingIndex("MaterialConstants").GetRootIndex();
			meshCullIA[1].Constant.Num32BitValuesToSet = m_MeshletDrawPass.GetConstantBindingIndex("MaterialConstants").GetNumConstants();
			meshCullIA[1].Constant.DestOffsetIn32BitValues = 0;
			meshCullIA[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			D3D12_COMMAND_SIGNATURE_DESC meshCullIADesc{};
			meshCullIADesc.pArgumentDescs = meshCullIA.data();
			meshCullIADesc.NumArgumentDescs = meshCullIA.size();
			meshCullIADesc.ByteStride = sizeof(MeshletCull_IA);
			m_diDevice->CreateCommandSignature(&meshCullIADesc, m_MeshletDrawPass.GetRootSignature(), IID_PPV_ARGS(m_MeshletCullSignature.GetAddressOf()));
			m_MeshletCull_IA_B = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(MeshletCull_IA) * MAX_INSTANCES, D3D12_HEAP_TYPE_DEFAULT, true));

			m_MeshInstanceBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(GPUProxy::StaticMeshInstance) * MAX_INSTANCES, D3D12_HEAP_TYPE_DEFAULT));

#ifdef USE_DEBUG
			m_MeshCull_Count_B.GetResource()->SetName(L"m_MeshCull_Count_B");
			m_MeshletCull_IA_B.GetResource()->SetName(L"m_MeshletCull_IA_B");
			m_MeshInstanceBuffer.GetResource()->SetName(L"m_MeshInstanceBuffer_VRAM");
#endif
		}

		void Renderer::RecordGPUDrivenRenderPipeline(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, const Rendering::GPUProxy::Camera& camera, uint32_t numStaticMeshes)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.m_DirectCmdList;

			std::array<ID3D12DescriptorHeap*, 2> heaps{ m_ResourceHeap.Get(), m_SamplerHeap.Get() };
			cmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

			GPUDrivenRenderConstants constants;
			constants.CameraFrustum = camera.m_Frustrum;
			constants.CameraView = camera.m_View;
			constants.MeshInstancesCount = numStaticMeshes;

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "MeshCull pass");

				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshCull_Count_B.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
				cmdList->ResourceBarrier(1, &barrier);

				// Reset the counting buffer used in the MeshCull pass
				uint32_t count = 0;
				frameContext.AddAllocation(count, m_MeshCull_Count_B, 0);
				frameContext.m_FrameAllocator.RecordAllocations(cmdList);

				barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshCull_Count_B.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				cmdList->ResourceBarrier(1, &barrier);

				barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletCull_IA_B.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				cmdList->ResourceBarrier(1, &barrier);

				m_MeshCullPass.Begin(cmdList, m_ResourceHeap, m_SamplerHeap);

				auto constants_Binding = m_MeshCullPass.GetConstantBindingIndex("Constants");
				auto meshInstances_Binding = m_MeshCullPass.GetBufferBindingIndex("MeshInstances");
				auto meshInstanceIndexCounter_Binding = m_MeshCullPass.GetBufferBindingIndex("MeshInstanceIndexCounter");
				auto meshletCullPass_IA_Binding = m_MeshCullPass.GetBufferBindingIndex("MeshletCullPass_IA");

				cmdList.SetComputeRoot32BitConstantsSafe(constants_Binding.GetRootIndex(), constants_Binding.GetNumConstants(), &constants, 0);
				cmdList.SetComputeRootShaderResourceViewSafe(meshInstances_Binding.GetRootIndex(), m_MeshInstanceBuffer.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootUnorderedAccessViewSafe(meshInstanceIndexCounter_Binding.GetRootIndex(), m_MeshCull_Count_B.GetResource()->GetGPUVirtualAddress());
				cmdList.SetComputeRootUnorderedAccessViewSafe(meshletCullPass_IA_Binding.GetRootIndex(), m_MeshletCull_IA_B.GetResource()->GetGPUVirtualAddress());
				cmdList->Dispatch(std::ceil(numStaticMeshes / 32.f), 1, 1);

				D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_MeshCull_Count_B.GetResource());
				cmdList->ResourceBarrier(1, &uavBarrier);
				barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshCull_Count_B.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				cmdList->ResourceBarrier(1, &barrier);


				uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_MeshletCull_IA_B.GetResource());
				cmdList->ResourceBarrier(1, &uavBarrier);
				barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletCull_IA_B.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				cmdList->ResourceBarrier(1, &barrier);

				m_DirectCommandQueue.ExecuteCommandList(cmdList, false); // TODO: Maybe don't execute the list here?
			}

			{
				PIXScopedEvent(cmdList.Get(), PIX_COLOR(0, 0, 255), "MeshletDraw pass");

				m_MeshletDrawPass.Begin(cmdList, m_ResourceHeap, m_SamplerHeap, { &renderTarget.GetDescriptor() }, { &depthStencilTarget.GetDescriptor() });

				auto constants_BindingMS = m_MeshletDrawPass.GetConstantBindingIndex("ConstantsMS");
				auto constants_BindingAS = m_MeshletDrawPass.GetConstantBindingIndex("ConstantsAS");
				DXM::Matrix vpMatrix = camera.m_View * camera.m_Projection;
				cmdList.SetGraphicsRoot32BitConstantsSafe(constants_BindingMS.GetRootIndex(), constants_BindingMS.GetNumConstants(), &vpMatrix, 0);
				//cmdList.SetGraphicsRoot32BitConstantsSafe(constants_BindingAS.GetRootIndex(), constants_BindingAS.GetNumConstants(), &constants, 0);

				struct PixelShaderConstantsData
				{
					uint32_t SamplerIndex;
					uint32_t MaterialBuffer;
					uint32_t PointLightBuffer;
					uint32_t SpotLightBuffer;
					uint32_t DirectionalLightBuffer;
					float Time;
					float pad[2];
				} pixelbindings;

				static float time = 0.f;
				time += 0.0005;
				pixelbindings.Time = time;

				pixelbindings.SamplerIndex = m_SamplerManager.GetSampler(aZero::Rendering::SamplerManager::Anisotropic_8x_Wrap).GetHeapIndex();
				pixelbindings.MaterialBuffer = m_ResourceManager.m_MaterialBufferView.GetHeapIndex();
				/*pixelbindings.PointLightBuffer = pointLightBufferIndex;
				pixelbindings.SpotLightBuffer = spotLightBufferIndex;
				pixelbindings.DirectionalLightBuffer = directionalLightBufferIndex;*/

				auto default_Phong_Constants_Binding = m_MeshletDrawPass.GetConstantBindingIndex("Default_Phong_Constants");
				cmdList.SetGraphicsRoot32BitConstantsSafe(default_Phong_Constants_Binding.GetRootIndex(), default_Phong_Constants_Binding.GetNumConstants(), &pixelbindings, 0);

				cmdList->RSSetScissorRects(1, &camera.m_RSInfo.ScizzorRect);
				cmdList->RSSetViewports(1, &camera.m_RSInfo.Viewport);

				cmdList->ExecuteIndirect(m_MeshletCullSignature.Get(), MAX_INSTANCES, m_MeshletCull_IA_B.GetResource(), 0, m_MeshCull_Count_B.GetResource(), 0);
				m_DirectCommandQueue.ExecuteCommandList(cmdList, false); // TODO: Maybe don't execute the list here?
			}
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

		// TODO: Change so not only the camera at index[0] will be used.
		void Renderer::Render(const Scene::Scene& scene, Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			auto& cmdList = frameContext.m_DirectCmdList;

			PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 0, 0), "Render scene");

			frameContext.RecordFrameAllocations(cmdList);
			m_DirectCommandQueue.ExecuteCommandList(cmdList, false); // TODO: Remove

			auto [staticMeshes, cameras] = scene.GetWorldRenderData();

			// TODO: Render to rtvs/dsv for each camera
			this->ClearRenderTarget(renderTarget);
			this->ClearDepthStencilTarget(depthStencilTarget);

			//if (staticMeshes.size() == 0 || cameras.size() == 0) { return; }

			frameContext.m_StaticMeshBuffer.Write(staticMeshes.data(), staticMeshes.size() * sizeof(staticMeshes[0]), 0);
			cmdList->CopyBufferRegion(m_MeshInstanceBuffer.GetResource(), 0, frameContext.m_StaticMeshBuffer.GetResource(), 0, staticMeshes.size() * sizeof(staticMeshes[0]));

			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);

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

		void Renderer::InitGPUDrivenRenderPipeline_NEW()
		{
			m_MeshCullCS_NEW.Compile(m_diCompiler, NEW_Pipeline::GetShaderDirectoryPath() + "MeshCull.cs.hlsl");
			m_MeshCullPass_NEW.CompileComputePass(m_diDevice, m_MeshCullCS_NEW);

			m_MeshletDrawAS_NEW.Compile(m_diCompiler, NEW_Pipeline::GetShaderDirectoryPath() + "MeshletDraw.as.hlsl");
			m_MeshletDrawMS_NEW.Compile(m_diCompiler, NEW_Pipeline::GetShaderDirectoryPath() + "MeshletDraw.ms.hlsl");
			m_MeshletDrawPS_NEW.Compile(m_diCompiler, NEW_Pipeline::GetShaderDirectoryPath() + "MeshletDraw.ps.hlsl");

			NEW_Pipeline::RenderPass::Desc passDesc;
			passDesc.RtvFormats.push_back(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			passDesc.DsvFormat = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_MeshletDrawPass_NEW.CompileMeshletPass(passDesc, m_diDevice, m_MeshletDrawAS_NEW, m_MeshletDrawMS_NEW, m_MeshletDrawPS_NEW);
		}

		void Renderer::RecordGPUDrivenRenderPipeline_NEW(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, const Rendering::GPUProxy::Camera& camera, uint32_t numStaticMeshes)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			RenderAPI::CommandList& cmdList = frameContext.m_DirectCmdList;
			cmdList.SetDescriptorHeaps(m_ResourceHeap, m_SamplerHeap);

			m_MeshCullPass_NEW.Begin(cmdList);



			// TODO: Set buffers and transition resources etc

			cmdList->Dispatch(std::ceil(numStaticMeshes / 32.f), 1, 1);

			m_MeshletDrawPass_NEW.Begin(cmdList);
			cmdList.OMSetRenderTargets({ renderTarget.GetDescriptor() }, depthStencilTarget.GetDescriptor());

			// TODO: Set buffers and transition resources etc

			// Executeindirect
		}
	}
}