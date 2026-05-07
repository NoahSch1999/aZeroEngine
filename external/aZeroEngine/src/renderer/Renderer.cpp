#include "Renderer.hpp"
#include "scene/Scene.hpp"
#include "WireframeRenderer.hpp"

#include "WinPixEventRuntime/pix3.h"

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

			this->InitPipeline();
		}

		void Renderer::InitPipeline()
		{
			this->InitMeshObjectCullPipeline();
			this->InitMeshletCullPipeline();
			this->InitMeshletDrawPipeline();

			/*Rendering::MeshShaderPass::Description msPassDesc;
			msPassDesc.ExecutionCount = 1;
			msPassDesc.Pipeline = &m_MeshletDrawPass;
			msPassDesc.RenderTargets.resize(1);
			msPassDesc.ClearRtvs.resize(1);
			m_RenderPasses.push_back(new Rendering::MeshShaderPass(std::move(msPassDesc)));*/
		}

		void Renderer::InitMeshObjectCullPipeline()
		{
			m_MeshObjectCullingCS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshObjectCulling.cs.hlsl"));
			Pipeline::ComputeShaderPass::Description icDesc;
			m_MeshObjectCullingPass.Compile(m_diDevice, icDesc, m_MeshObjectCullingCS);
		}

		void Renderer::InitMeshletCullPipeline()
		{
			m_MeshletCullingCS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletCulling.cs.hlsl"));
			Pipeline::ComputeShaderPass::Description icDesc;
			m_MeshletCullingPass.Compile(m_diDevice, icDesc, m_MeshletCullingCS);

			std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2> indirectArgs;
			indirectArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			indirectArgs[0].Constant.RootParameterIndex = m_MeshletCullingPass.GetConstantBindingIndex("Instance").GetRootIndex();
			indirectArgs[0].Constant.Num32BitValuesToSet = m_MeshletCullingPass.GetConstantBindingIndex("Instance").GetNumConstants();
			indirectArgs[0].Constant.DestOffsetIn32BitValues = 0;
			indirectArgs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

			struct IndirectCommand
			{
				uint32_t instanceID;
				D3D12_DISPATCH_ARGUMENTS dispatchArgs;
			};

			D3D12_COMMAND_SIGNATURE_DESC indirectArgsDesc{};
			indirectArgsDesc.pArgumentDescs = indirectArgs.data();
			indirectArgsDesc.NumArgumentDescs = indirectArgs.size();
			indirectArgsDesc.ByteStride = sizeof(IndirectCommand);

			m_diDevice->CreateCommandSignature(&indirectArgsDesc, m_MeshletCullingPass.GetRootSignature(), IID_PPV_ARGS(m_MeshObjectCullSignature.GetAddressOf()));

			m_MeshObjectCullingBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(IndirectCommand) * MAX_INSTANCES, D3D12_HEAP_TYPE_DEFAULT, true));
			m_MeshObjectCullingUAV = RenderAPI::UnorderedAccessView(m_diDevice, m_ResourceHeap, m_MeshObjectCullingBuffer, MAX_INSTANCES, sizeof(IndirectCommand), 0);

			m_PassedMeshCountBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT, true, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
			m_PassedMeshCountUAV = RenderAPI::UnorderedAccessView(m_diDevice, m_ResourceHeap, m_PassedMeshCountBuffer, 1, sizeof(uint32_t), 0);

#ifdef USE_DEBUG
			m_MeshObjectCullingBuffer.GetResource()->SetName(L"m_MeshObjectCullingBuffer");
			m_PassedMeshCountBuffer.GetResource()->SetName(L"m_PassedMeshCountBuffer");
#endif
		}

		void Renderer::InitMeshletDrawPipeline()
		{
			m_MeshletDrawMS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletDraw.ms.hlsl"));
			m_MeshletDrawPS.CompileFromFile(m_Compiler, PROJECT_DIRECTORY + std::string("shaderSource/MeshletDraw.ps.hlsl"));

			Pipeline::MeshShaderPass::Description pipelineDesc;
			pipelineDesc.m_RenderTargets.push_back({ DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "ColorTarget" });
			pipelineDesc.m_DepthStencil.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			m_MeshletDrawPass.Compile(m_diDevice, pipelineDesc, {}, m_MeshletDrawMS, &m_MeshletDrawPS);

			std::array<D3D12_INDIRECT_ARGUMENT_DESC, 1> meshletDrawArguments;
			meshletDrawArguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

			D3D12_COMMAND_SIGNATURE_DESC meshletDrawDesc{};
			meshletDrawDesc.NumArgumentDescs = meshletDrawArguments.size();
			meshletDrawDesc.pArgumentDescs = meshletDrawArguments.data();
			meshletDrawDesc.ByteStride = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);

			m_diDevice->CreateCommandSignature(&meshletDrawDesc, nullptr, IID_PPV_ARGS(m_MeshletDrawSignature.GetAddressOf()));

			m_MeshletDrawArgumentBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(D3D12_DISPATCH_MESH_ARGUMENTS) * 1, D3D12_HEAP_TYPE_DEFAULT, true, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
			m_MeshletDrawArgumentUAV = RenderAPI::UnorderedAccessView(m_diDevice, m_ResourceHeap, m_MeshletDrawArgumentBuffer, 1, sizeof(D3D12_DISPATCH_MESH_ARGUMENTS), 0);

			struct MeshletCulling_To_MeshShader_Data
			{
				DXM::Matrix Transform;
				uint32_t BatchID;

				uint32_t VertCount;
				uint32_t VertOffset;
				uint32_t PrimCount;
				uint32_t PrimOffset;

				uint32_t PrimitiveBuffer;
				uint32_t IndicesBuffer;
				uint32_t PositionBuffer;
				uint32_t VertexDataBuffer;
			};

			m_MeshletInstanceBuffer = RenderAPI::Buffer(m_diDevice, RenderAPI::Buffer::Desc(sizeof(MeshletCulling_To_MeshShader_Data) * MAX_MESHLETS, D3D12_HEAP_TYPE_DEFAULT, true));
			m_MeshletInstanceUAV = RenderAPI::UnorderedAccessView(m_diDevice, m_ResourceHeap, m_MeshletInstanceBuffer, MAX_MESHLETS, sizeof(MeshletCulling_To_MeshShader_Data), 0);
			m_MeshletInstanceSRV = RenderAPI::ShaderResourceView(m_diDevice, m_ResourceHeap, m_MeshletInstanceBuffer, MAX_MESHLETS, sizeof(MeshletCulling_To_MeshShader_Data), 0);

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

		bool Renderer::TryBeginFrame()
		{
			const bool hasNewFrameStarted = AdvanceFrameIfReady();
			if (hasNewFrameStarted)
			{
				m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());
				m_FrameCount++;
				m_ResourceRecycler.SetFrameIndex(m_FrameIndex);
				m_ResourceRecycler.Clear();
				m_WireframeRenderer->BeginFrame(m_FrameIndex);
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
		
		void Renderer::RecordMeshObjectCullingPass(const BindingConstants& bindings, uint32_t numStaticMeshes)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			PIXScopedEvent(frameContext.m_DirectCmdList.Get(), PIX_COLOR(0, 0, 255), "Mesh object culling pass");

			auto& cmdList = frameContext.m_DirectCmdList;

			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_PassedMeshCountBuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->ResourceBarrier(1, &barrier);

			uint32_t count = 0;
			frameContext.AddAllocation(count, m_PassedMeshCountBuffer, 0);
			frameContext.m_FrameAllocator.RecordAllocations(cmdList);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_PassedMeshCountBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->ResourceBarrier(1, &barrier);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshObjectCullingBuffer.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->ResourceBarrier(1, &barrier);

			m_MeshObjectCullingPass.Begin(cmdList, m_ResourceHeap, m_SamplerHeap);

			struct MeshObjectCullingConstants
			{
				uint32_t NumMeshObjects;
				uint32_t IndirectArgumentMeshCullingBuffer;
				uint32_t PassedMeshesCounterBuffer;
			} passConstants;

			passConstants.NumMeshObjects = numStaticMeshes;
			passConstants.IndirectArgumentMeshCullingBuffer = m_MeshObjectCullingUAV.GetHeapIndex();
			passConstants.PassedMeshesCounterBuffer = m_PassedMeshCountUAV.GetHeapIndex();

			auto bindingsBinding = m_MeshObjectCullingPass.GetConstantBindingIndex("Bindings");
			cmdList.SetComputeRoot32BitConstantsSafe(bindingsBinding.GetRootIndex(), bindingsBinding.GetNumConstants(), &bindings, 0);

			auto passConstantsBinding = m_MeshObjectCullingPass.GetConstantBindingIndex("PassConstants");
			cmdList.SetComputeRoot32BitConstantsSafe(passConstantsBinding.GetRootIndex(), passConstantsBinding.GetNumConstants(), &passConstants, 0);
			cmdList->Dispatch(std::ceil(numStaticMeshes / 64.f), 1, 1);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshObjectCullingBuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			cmdList->ResourceBarrier(1, &barrier);
			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);
		}

		void Renderer::RecordMeshLetCullingPass(const BindingConstants& bindings)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			PIXScopedEvent(frameContext.m_DirectCmdList.Get(), PIX_COLOR(0, 0, 255), "Meshlet culling pass");

			auto& cmdList = frameContext.m_DirectCmdList;

			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDrawArgumentBuffer.GetResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
			frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);

			D3D12_DISPATCH_MESH_ARGUMENTS meshShaderDispatchArgs = { 0,1,1 };
			frameContext.AddAllocation(meshShaderDispatchArgs, m_MeshletDrawArgumentBuffer, 0);
			frameContext.m_FrameAllocator.RecordAllocations(cmdList);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDrawArgumentBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->ResourceBarrier(1, &barrier);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_PassedMeshCountBuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			cmdList->ResourceBarrier(1, &barrier);

			auto bindingsBinding = m_MeshletCullingPass.GetConstantBindingIndex("Bindings");
			m_MeshletCullingPass.Begin(cmdList, m_ResourceHeap, m_SamplerHeap);
			cmdList.SetComputeRoot32BitConstantsSafe(bindingsBinding.GetRootIndex(), bindingsBinding.GetNumConstants(), &bindings, 0);

			// TODO: Handle so we only dispatch up to the number of meshes that passed the first pass
			cmdList->ExecuteIndirect(m_MeshObjectCullSignature.Get(), MAX_INSTANCES, m_MeshObjectCullingBuffer.GetResource(), 0, m_PassedMeshCountBuffer.GetResource(), 0);
			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);
		}

		void Renderer::ClearRenderSurfaces(const Scene::RenderData::Camera& camera)
		{
			FrameContext& frameContext = this->GetCurrentContext();
			PIXScopedEvent(frameContext.m_DirectCmdList.Get(), PIX_COLOR(0, 255, 0), "Clear render surfaces");

			std::array<ID3D12DescriptorHeap*, 2> heaps{ m_ResourceHeap.Get(), m_SamplerHeap.Get() };
			frameContext.m_DirectCmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

			if (camera.m_RenderTarget.has_value())
			{
				if (camera.m_ClearRenderTarget)
				{
					auto& target = *camera.m_RenderTarget.value();
					auto& texture = target.GetTexture();
					if (texture.GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
					{
						auto barrier = texture.CreateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET);
						frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);
					}
					frameContext.m_DirectCmdList->ClearRenderTargetView(target.GetCpuHandle(), target.GetClearValue().Color, 0, nullptr);
				}
			}

			if (camera.m_DepthStencilTarget.has_value())
			{
				D3D12_CLEAR_FLAGS clearFlags = static_cast<D3D12_CLEAR_FLAGS>(0);
				if (camera.m_ClearDepthTarget)
				{
					clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
				}

				if (camera.m_ClearStencilTarget)
				{
					clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
				}

				if (clearFlags)
				{
					auto& target = *camera.m_DepthStencilTarget.value();
					auto& texture = target.GetTexture();
					if (texture.GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
					{
						auto barrier = texture.CreateTransition(D3D12_RESOURCE_STATE_DEPTH_WRITE);
						frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);
					}
					const auto value = target.GetClearValue().DepthStencil;
					frameContext.m_DirectCmdList->ClearDepthStencilView(target.GetCpuHandle(), clearFlags, value.Depth, value.Stencil, 0, nullptr);
				}
			}

			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);
		}

		// TODO: Change so not only the camera at index[0] will be used.
		void Renderer::Render(const Scene::Scene& scene, Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			PIXScopedEvent(frameContext.m_DirectCmdList.Get(), PIX_COLOR(255, 0, 0), "Render scene");

			frameContext.RecordFrameAllocations(frameContext.m_DirectCmdList);
			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);

			std::array<ID3D12DescriptorHeap*, 2> heaps{ m_ResourceHeap.Get(), m_SamplerHeap.Get() };
			frameContext.m_DirectCmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

			if (renderTarget.GetTexture().GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				auto barrier = renderTarget.GetTexture().CreateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET);
				frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);
			}
			frameContext.m_DirectCmdList->ClearRenderTargetView(renderTarget.GetCpuHandle(), renderTarget.GetClearValue().Color, 0, nullptr);

			if (depthStencilTarget.GetTexture().GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
			{
				auto barrier = depthStencilTarget.GetTexture().CreateTransition(D3D12_RESOURCE_STATE_DEPTH_WRITE);
				frameContext.m_DirectCmdList->ResourceBarrier(1, &barrier);
			}
			const auto value = depthStencilTarget.GetClearValue().DepthStencil;
			frameContext.m_DirectCmdList->ClearDepthStencilView(depthStencilTarget.GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, value.Depth, value.Stencil, 0, nullptr);
			m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList, false);

			auto [staticMeshes, cameras, cameraRasterInfo] = scene.GetWorldRenderData();
			if (staticMeshes.size() == 0 || cameras.size() == 0) { return; }

			frameContext.m_StaticMeshBuffer.Write(staticMeshes.data(), staticMeshes.size() * sizeof(staticMeshes[0]), 0);
			frameContext.m_CameraBuffer.Write(cameras.data(), cameras.size() * sizeof(cameras[0]), 0);

			BindingConstants constants;
			constants.InstanceBuffer = frameContext.m_StaticMeshDescriptor.GetHeapIndex();
			constants.MeshBuffer = m_ResourceManager.m_MeshBufferView.GetHeapIndex();
			constants.CameraBuffer = frameContext.m_CameraDescriptor.GetHeapIndex();
			constants.CameraID = 0;
			constants.IndirectArgumentMeshletCullingBuffer = m_MeshletDrawArgumentUAV.GetHeapIndex();
			constants.MeshletInstanceBuffer = m_MeshletInstanceUAV.GetHeapIndex();

			this->RecordMeshObjectCullingPass(constants, staticMeshes.size());

			this->RecordMeshLetCullingPass(constants);

			this->RecordMeshDrawingPass_New(constants, 
				renderTarget, depthStencilTarget, 
				cameraRasterInfo[0].Viewport, cameraRasterInfo[0].ScizzorRect,
				frameContext.m_PointLightDescriptor.GetHeapIndex(), 
				frameContext.m_SpotLightDescriptor.GetHeapIndex(), 
				frameContext.m_DirectionalLightDescriptor.GetHeapIndex()
			);

		}

		void Renderer::RecordMeshDrawingPass_New(
			const BindingConstants& bindings,
			Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget,
			const D3D12_VIEWPORT& viewport, const D3D12_RECT& scizzorRect,
			uint32_t pointLightBufferIndex,
			uint32_t spotLightBufferIndex,
			uint32_t directionalLightBufferIndex)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			PIXScopedEvent(frameContext.m_DirectCmdList.Get(), PIX_COLOR(0, 0, 255), "Meshlet drawing pass");

			auto& cmdList = frameContext.m_DirectCmdList;

			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDrawArgumentBuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			cmdList->ResourceBarrier(1, &barrier);

			std::vector<RenderAPI::Descriptor*> renderTargets;
			renderTargets.push_back(&renderTarget.GetDescriptor());

			RenderAPI::Descriptor* dsv = &depthStencilTarget.GetDescriptor();
			m_MeshletDrawPass.Begin(cmdList, m_ResourceHeap, m_SamplerHeap, renderTargets, dsv);

			BindingConstants b = bindings;
			b.MeshletInstanceBuffer = m_MeshletInstanceSRV.GetHeapIndex();
			auto msBindings = m_MeshletDrawPass.GetConstantBindingIndex("Bindings");
			cmdList.SetGraphicsRoot32BitConstantsSafe(msBindings.GetRootIndex(), msBindings.GetNumConstants(), &b, 0);

			struct PixelShaderConstantsData
			{
				uint32_t SamplerIndex;
				uint32_t MaterialBuffer;
				uint32_t PointLightBuffer;
				uint32_t SpotLightBuffer;
				uint32_t DirectionalLightBuffer;
				float Time;
			} pixelbindings;

			static float time = 0.f;
			time += 0.0005;
			pixelbindings.Time = time;

			pixelbindings.SamplerIndex = m_SamplerManager.GetSampler(aZero::Rendering::SamplerManager::Anisotropic_8x_Wrap).GetHeapIndex();
			pixelbindings.MaterialBuffer = m_ResourceManager.m_MaterialBufferView.GetHeapIndex();
			pixelbindings.PointLightBuffer = pointLightBufferIndex;
			pixelbindings.SpotLightBuffer = spotLightBufferIndex;
			pixelbindings.DirectionalLightBuffer = directionalLightBufferIndex;

			auto psConstants = m_MeshletDrawPass.GetConstantBindingIndex("PixelShaderConstants");
			cmdList.SetGraphicsRoot32BitConstantsSafe(psConstants.GetRootIndex(), psConstants.GetNumConstants(), &pixelbindings, 0);

			cmdList->RSSetScissorRects(1, &scizzorRect);
			cmdList->RSSetViewports(1, &viewport);

			cmdList->ExecuteIndirect(m_MeshletDrawSignature.Get(), 1, m_MeshletDrawArgumentBuffer.GetResource(), 0, nullptr, 0);
			m_DirectCommandQueue.ExecuteCommandList(cmdList, false);
		}

		void Renderer::FlushGPU()
		{
			m_DirectCommandQueue.Flush();

			// todo When we're also using other types of queues we need to add them here and do some other stuff
		}

		void Renderer::CopyRenderTargetToSwapChain(RenderAPI::SwapChain& swapChain, Rendering::RenderTarget& renderTarget)
		{
			FrameContext& frameContext = this->GetCurrentContext();

			std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
			preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, swapChain.GetFrameBackBuffer()});
			preCopyBarriers.push_back({ renderTarget.GetTexture().GetState(), D3D12_RESOURCE_STATE_COPY_SOURCE, renderTarget.GetTexture().GetResource()});

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

		Rendering::RenderTarget Renderer::CreateRenderTarget(const Rendering::RenderTarget::Desc& desc)
		{
			return Rendering::RenderTarget(desc, m_diDevice, m_RTVHeap, &m_ResourceRecycler);
		}

		Rendering::DepthStencilTarget Renderer::CreateDepthStencilTarget(const Rendering::DepthStencilTarget::Desc& desc)
		{
			return Rendering::DepthStencilTarget(desc, m_diDevice, m_DSVHeap, &m_ResourceRecycler);
		}

		Rendering::WireframeRenderer& Renderer::GetWireframeRenderer() { return *m_WireframeRenderer.get(); }

		/*void Renderer::ExecuteRenderPasses()
		{
			FrameContext& context = this->GetCurrentContext();
			RenderAPI::CommandList& cmdList = context.m_DirectCmdList;
			for (const auto pass : m_RenderPasses)
			{
				pass->Execute(m_DirectCommandQueue, context.m_DirectCmdList, m_ResourceHeap, m_SamplerHeap);
			}
		}*/
	}
}