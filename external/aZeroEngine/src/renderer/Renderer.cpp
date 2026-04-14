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
			m_AmpShader.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/AmpShader.as.hlsl"));
			m_MeshShader.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshShader.ms.hlsl"));
			m_PixelShader.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/PixelShader.ps.hlsl"));

			Pipeline::MeshShaderPass::Description pipelineDesc;
			pipelineDesc.m_RenderTargets.push_back({ DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "ColorTarget" });
			pipelineDesc.m_DepthStencil.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_Pipeline.Compile(m_diDevice, pipelineDesc, &m_AmpShader, m_MeshShader, &m_PixelShader);
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
			frameContext.SetLatestSignal(m_DirectCommandQueue.GetLatestSignal());
		}

		void Renderer::Render(const Scene::SceneNew& scene, std::optional<Rendering::RenderTarget*> renderTarget, std::optional<Rendering::DepthTarget*> depthTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			// Perform uploads for all updated/new assets and other stagings
			frameContext.RecordFrameAllocations(frameContext.m_DirectCmdList);
			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);

			// todo Do rendering
			const auto& staticMeshInstances = scene.GetProxy()->m_StaticMeshes.GetData();
			frameContext.m_StaticMeshBuffer.Write(staticMeshInstances.data(), staticMeshInstances.size() * sizeof(staticMeshInstances[0]), 0);

			const auto& pointLights = scene.GetProxy()->m_PointLights.GetData();
			frameContext.m_PointLightBuffer.Write(pointLights.data(), pointLights.size() * sizeof(pointLights[0]), 0);

			const auto& spotLights = scene.GetProxy()->m_SpotLights.GetData();
			frameContext.m_SpotLightBuffer.Write(spotLights.data(), spotLights.size() * sizeof(spotLights[0]), 0);

			const auto& directionalLights = scene.GetProxy()->m_DirectionalLights.GetData();
			frameContext.m_DirectionalLightBuffer.Write(directionalLights.data(), directionalLights.size() * sizeof(directionalLights[0]), 0);

			const auto& cameras = scene.GetProxy()->m_Cameras.GetData();
			frameContext.m_CameraBuffer.Write(cameras.data(), cameras.size() * sizeof(cameras[0]), 0);

			auto rootConstants = m_Pipeline.GetConstantBindingIndex("Bindings");
			auto rootIndex = rootConstants.GetRootIndex();
			auto rootConstantsSize = rootConstants.GetNumConstants();

			// TODO: Dispatch amp + mesh shaders
			// Amp shader culls and produces work
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
						uint32_t InstanceID;
						uint32_t CameraID;
					} constants;

					constants.InstanceBuffer = frameContext.m_StaticMeshDescriptor.GetHeapIndex();
					constants.MeshBuffer = m_ResourceManager.m_MeshBufferView.GetHeapIndex();
					constants.CameraBuffer = frameContext.m_CameraDescriptor.GetHeapIndex();
					constants.InstanceID = 0;
					constants.CameraID = i;

					m_Pipeline.Begin(frameContext.m_DirectCmdList, m_ResourceHeapNew, m_SamplerHeapNew, { &renderTarget.value()->GetTargetDescriptor() }, & depthTarget.value()->GetTargetDescriptor());
					frameContext.m_DirectCmdList->SetGraphicsRoot32BitConstants(rootIndex, rootConstantsSize, &constants, 0);

					frameContext.m_DirectCmdList->DispatchMesh(staticMeshInstances.size(), 1, 1); // ?
				}
			}
		}

		void Renderer::FlushGPU()
		{
			m_DirectCommandQueue.Flush();

			// todo When we're also using other types of queues we need to add them here and do some other stuff
		}

		void Renderer::CopyTextureToTexture(RenderAPI::Texture2D& dstTexture, RenderAPI::Texture2D& srcTexture)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			ID3D12Resource* dstResource = dstTexture.GetResource();
			ID3D12Resource* srcResource = srcTexture.GetResource();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, dstResource });
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, preCopyBarriers);

			frameContext.m_DirectCmdList->CopyResource(dstTexture.GetResource(), srcTexture.GetResource());

			std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, dstResource });
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, postCopyBarriers);

			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);
		}

		void Renderer::CopyTextureToTexture(ID3D12Resource* dstResource, ID3D12Resource* srcResource)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, dstResource });
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, preCopyBarriers);

			frameContext.m_DirectCmdList->CopyResource(dstResource, srcResource);

			std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, dstResource });
			postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON, srcResource });

			RenderAPI::TransitionResources(frameContext.m_DirectCmdList, postCopyBarriers);

			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);
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

			m_ResourceManager.UpdateRenderState(m_NewResourceRecycler, m_ResourceHeapNew, *texture);
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