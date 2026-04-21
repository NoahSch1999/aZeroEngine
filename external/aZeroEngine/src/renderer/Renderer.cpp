#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "RenderSurface.hpp"

namespace aZero
{
	namespace Rendering
	{
		Renderer::Renderer(ID3D12DeviceX* device, uint32_t bufferCount, IDxcCompilerX& compiler)
			:m_Compiler(compiler), m_diDevice(device), m_BufferCount(bufferCount)
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
			device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData));

			if (featureData.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
				throw std::runtime_error("Device doesn't support mesh shaders.");
			}

			m_NewResourceRecycler = RenderAPI::ResourceRecycler(bufferCount);

			m_DirectCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_CopyCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY);
			m_ComputeCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

			m_ResourceHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, true);
			m_SamplerHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
			m_RTVHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
			m_DSVHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);

			// TODO: DEFINE A MOVE-CONSTRUCTOR FOR FRAMECONTEXT
			// OTHERWISE THIS WILL CRASH IF WE DONT RESERVE
			m_FrameContexts.reserve(bufferCount);
			for (int32_t i = 0; i < bufferCount; i++)
			{
				m_FrameContexts.emplace_back(device, m_ResourceHeapNew, m_NewResourceRecycler);
			}

			m_SamplerManager = SamplerManager(device, m_SamplerHeapNew);

			m_ResourceManager = ResourceManager(device, &m_NewResourceRecycler, m_ResourceHeapNew);

			this->InitPipeline();
		}

		void Renderer::InitPipeline()
		{
			this->InitMeshletCullPipeline();
			this->InitMeshletDrawPipeline();
		}

		void Renderer::InitMeshletCullPipeline()
		{
			m_MeshletCullingCS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletCulling.cs.hlsl"));
			Pipeline::ComputeShaderPass::Description icDesc;
			m_MeshletCullingPass.Compile(m_diDevice, icDesc, m_MeshletCullingCS);
		}

		struct DummyToFix
		{
			D3D12_INDIRECT_ARGUMENT_TYPE Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			D3D12_DISPATCH_MESH_ARGUMENTS MeshShaderDispatchArgs = { 1,1,1 };
		};

		void Renderer::InitMeshletDrawPipeline()
		{
			m_MeshletDrawMS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletDraw.ms.hlsl"));
			m_MeshletDrawPS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletDraw.ps.hlsl"));

			Pipeline::MeshShaderPass::Description pipelineDesc;
			pipelineDesc.m_RenderTargets.push_back({ DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "ColorTarget" });
			pipelineDesc.m_DepthStencil.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_MeshletDrawPass.Compile(m_diDevice, pipelineDesc, {}, m_MeshletDrawMS, &m_MeshletDrawPS);

			std::array<D3D12_INDIRECT_ARGUMENT_DESC, 1> meshletDrawArguments;

			DummyToFix dispatchArgs;
			memcpy(&meshletDrawArguments[0], (void*)&dispatchArgs, sizeof(dispatchArgs));

			D3D12_COMMAND_SIGNATURE_DESC meshletDrawDesc{};
			meshletDrawDesc.ByteStride = sizeof(D3D12_INDIRECT_ARGUMENT_DESC);
			meshletDrawDesc.NumArgumentDescs = meshletDrawArguments.size();
			meshletDrawDesc.pArgumentDescs = meshletDrawArguments.data();

			m_diDevice->CreateCommandSignature(&meshletDrawDesc, nullptr, IID_PPV_ARGS(m_MeshletDrawSignature.GetAddressOf()));

			m_MeshletDrawArgumentBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(D3D12_DISPATCH_MESH_ARGUMENTS) * 1, D3D12_HEAP_TYPE_DEFAULT, true, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
			m_MeshletDrawArgumentUAV = RenderAPI::UnorderedAccessView(m_diDevice, m_ResourceHeapNew, m_MeshletDrawArgumentBuffer, 1, sizeof(D3D12_DISPATCH_MESH_ARGUMENTS), 0);

			struct MeshletCulling_To_MeshShader_Data
			{
				uint32_t InstanceID;
				uint32_t LocalMeshletIndex;
			};

			m_MeshletInstanceBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(MeshletCulling_To_MeshShader_Data) * MAX_INSTANCES, D3D12_HEAP_TYPE_DEFAULT, true));
			m_MeshletInstanceUAV = RenderAPI::UnorderedAccessView(m_diDevice, m_ResourceHeapNew, m_MeshletInstanceBuffer, MAX_INSTANCES, sizeof(MeshletCulling_To_MeshShader_Data), 0);

#ifdef USE_DEBUG
			m_MeshletDrawArgumentBuffer.GetResource()->SetName(L"m_MeshletDrawArgumentBuffer");
			m_MeshletInstanceBuffer.GetResource()->SetName(L"m_MeshletInstanceBuffer");
#endif
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

		bool Renderer::BeginFrame()
		{
			const bool hasNewFrameStarted = AdvanceFrameIfReady();
			if (hasNewFrameStarted)
			{
				m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());
				m_FrameCount++;
				m_NewResourceRecycler.SetFrameIndex(m_FrameIndex);
				m_NewResourceRecycler.Clear();
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

		void Renderer::Render(const Scene::SceneNew& scene, std::optional<Rendering::RenderTarget*> renderTarget, std::optional<Rendering::DepthTarget*> depthTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			std::array<ID3D12DescriptorHeap*, 2> heaps{ m_ResourceHeapNew.Get(), m_SamplerHeapNew.Get() };
			frameContext.m_DirectCmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

			if (renderTarget.has_value())
			{
				const DXM::Vector4 rtvClearColor = renderTarget.value()->GetDesc().colorClearValue;
				const FLOAT clearColor[4] = { rtvClearColor.x, rtvClearColor.y, rtvClearColor.z, rtvClearColor.w };
				frameContext.m_DirectCmdList->ClearRenderTargetView(renderTarget.value()->GetTargetDescriptor().GetCpuHandle(), clearColor, 0, nullptr);
			}

			if (depthTarget.has_value())
			{
				frameContext.m_DirectCmdList->ClearDepthStencilView(depthTarget.value()->GetTargetDescriptor().GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
					depthTarget.value()->GetDesc().depthClearValue,
					depthTarget.value()->GetDesc().stencilClearValue, 0, nullptr);
			}

			// Perform uploads for all updated/new assets and other stagings
			frameContext.RecordFrameAllocations(frameContext.m_DirectCmdList);
			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);

			// todo Save start offset for the instances inside the buffer so each scene has its own portion
			const auto& staticMeshInstances = scene.GetProxy()->m_StaticMeshes.GetData();
			frameContext.m_StaticMeshBuffer.Write(staticMeshInstances.data(), staticMeshInstances.size() * sizeof(staticMeshInstances[0]), 0);

			const auto& pointLights = scene.GetProxy()->m_PointLights.GetData();
			frameContext.m_PointLightBuffer.Write(pointLights.data(), pointLights.size() * sizeof(pointLights[0]), 0);

			const auto& spotLights = scene.GetProxy()->m_SpotLights.GetData();
			frameContext.m_SpotLightBuffer.Write(spotLights.data(), spotLights.size() * sizeof(spotLights[0]), 0);

			const auto& directionalLights = scene.GetProxy()->m_DirectionalLights.GetData();
			frameContext.m_DirectionalLightBuffer.Write(directionalLights.data(), directionalLights.size() * sizeof(directionalLights[0]), 0);

			auto rootConstants = m_MeshletDrawPass.GetConstantBindingIndex("Bindings");
			auto rootIndex = rootConstants.GetRootIndex();
			auto rootConstantsSize = rootConstants.GetNumConstants();

			struct Pixelbindings
			{
				uint32_t SamplerIndex;
				uint32_t MaterialBuffer;
			} pixelbindings;
			auto pixelbindingsRoot = m_MeshletDrawPass.GetConstantBindingIndex("PixelShaderConstants");
			pixelbindings.SamplerIndex = m_SamplerManager.GetSampler(aZero::Rendering::SamplerManager::Anisotropic_8x_Wrap).GetHeapIndex();
			pixelbindings.MaterialBuffer = m_ResourceManager.m_MaterialBufferView.GetHeapIndex();

			auto meshletCullingCS_Bindings = m_MeshletCullingPass.GetConstantBindingIndex("Bindings");

			uint32_t lastCameraIndex = 0;
			const auto& cameras = scene.GetProxy()->m_Cameras.GetData();
			for (uint32_t i = 0; i < cameras.size(); i++)
			{
				const auto& camera = cameras[i];
				if (camera.m_IsActive)
				{
					struct BindingConstants
					{
						uint32_t InstanceBuffer;
						uint32_t MeshBuffer;
						uint32_t CameraBuffer;
						uint32_t CameraID;
						uint32_t IndirectArgumentBuffer;
						uint32_t MeshletInstanceBuffer;
					} constants;

					D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDrawArgumentBuffer.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
					frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);

					D3D12_DISPATCH_MESH_ARGUMENTS meshShaderDispatchArgs = { 0,1,1 };
					frameContext.AddAllocation(meshShaderDispatchArgs, m_MeshletDrawArgumentBuffer, 0);
					frameContext.m_FrameAllocator.RecordAllocations(frameContext.m_DirectCmdList);

					const auto gpuCamera = camera.CreateGPUVersion();
					frameContext.m_CameraBuffer.Write(&gpuCamera, sizeof(gpuCamera), sizeof(gpuCamera) * lastCameraIndex);

					constants.InstanceBuffer = frameContext.m_StaticMeshDescriptor.GetHeapIndex();
					constants.MeshBuffer = m_ResourceManager.m_MeshBufferView.GetHeapIndex();
					constants.CameraBuffer = frameContext.m_CameraDescriptor.GetHeapIndex();
					constants.CameraID = lastCameraIndex;
					constants.IndirectArgumentBuffer = m_MeshletDrawArgumentUAV.GetHeapIndex();
					constants.MeshletInstanceBuffer = m_MeshletInstanceUAV.GetHeapIndex();
					lastCameraIndex++;

					barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDrawArgumentBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);

					m_MeshletCullingPass.Begin(frameContext.m_DirectCmdList, m_ResourceHeapNew, m_SamplerHeapNew);
					frameContext.m_DirectCmdList.SetComputeRoot32BitConstantsSafe(meshletCullingCS_Bindings.GetRootIndex(), meshletCullingCS_Bindings.GetNumConstants(), &constants, 0);
					//frameContext.m_DirectCmdList->Dispatch(2, 1, 1);
					frameContext.m_DirectCmdList->Dispatch(473, 1, 1);


					barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDrawArgumentBuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);

					m_MeshletDrawPass.Begin(frameContext.m_DirectCmdList, m_ResourceHeapNew, m_SamplerHeapNew, { &renderTarget.value()->GetTargetDescriptor() }, &depthTarget.value()->GetTargetDescriptor());
					frameContext.m_DirectCmdList.SetGraphicsRoot32BitConstantsSafe(rootIndex, rootConstantsSize, &constants, 0);
					frameContext.m_DirectCmdList.SetGraphicsRoot32BitConstantsSafe(pixelbindingsRoot.GetRootIndex(), pixelbindingsRoot.GetNumConstants(), &pixelbindings, 0);
					
					frameContext.m_DirectCmdList->RSSetScissorRects(1, &camera.m_ScizzorRect);
					frameContext.m_DirectCmdList->RSSetViewports(1, &camera.m_Viewport);

					frameContext.m_DirectCmdList->ExecuteIndirect(m_MeshletDrawSignature.Get(), 1, m_MeshletDrawArgumentBuffer.GetResource(), 0, nullptr, 0);
					m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);
				}
			}
		}

		void Renderer::FlushGPU()
		{
			m_DirectCommandQueue.Flush();

			// todo When we're also using other types of queues we need to add them here and do some other stuff
		}

		void Renderer::CopyTextureToTexture(ID3D12Resource* dstResource, ID3D12Resource* srcResource)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, dstResource });
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, preCopyBarriers);

			frameContext.m_DirectCmdList->CopyResource(dstResource, srcResource);

			std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, dstResource });
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, postCopyBarriers);

			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);
		}

		void Renderer::UpdateRenderState(Asset::Mesh* mesh)
		{
			if (!mesh)
			{
				DEBUG_PRINT("Invalid mesh handle.");
				throw;
			}

			FrameContext& context = this->GetCurrentContext();
			m_ResourceManager.UpdateRenderState(m_diDevice, context.m_DirectCmdList, context.m_FrameAllocator, m_NewResourceRecycler, m_ResourceHeapNew, *mesh);
			m_DirectCommandQueue.ExecuteCommandList(context.m_DirectCmdList);
		}

		void Renderer::UpdateRenderState(Asset::Material* material)
		{
			if (!material)
			{
				DEBUG_PRINT("Invalid material handle.");
				throw;
			}

			m_ResourceManager.UpdateRenderState(this->GetCurrentContext().m_FrameAllocator, m_NewResourceRecycler, m_ResourceHeapNew, *material);
		}

		void Renderer::UpdateRenderState(Asset::Texture* texture)
		{
			if (!texture)
			{
				DEBUG_PRINT("Invalid texture handle.");
				throw;
			}

			FrameContext& context = this->GetCurrentContext();
			m_ResourceManager.UpdateRenderState(m_diDevice, context.m_DirectCmdList, m_NewResourceRecycler, m_ResourceHeapNew, *texture);
			m_DirectCommandQueue.ExecuteCommandList(context.m_DirectCmdList);
		}

		void Renderer::RemoveRenderState(Asset::Mesh* mesh)
		{
			//m_ResourceManager.
		}

		void Renderer::RemoveRenderState(Asset::Material* material)
		{
			//m_ResourceManager.
		}

		void Renderer::RemoveRenderState(Asset::Texture* texture)
		{
			//m_ResourceManager.
		}
	}
}