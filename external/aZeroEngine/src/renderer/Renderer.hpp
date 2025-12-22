#pragma once
#include "graphics_api/CommandQueue.hpp"
#include "renderer/render_graph/RenderGraph.hpp"
#include "ecs/aZeroECS.hpp"
#include "scene/SceneManager.hpp"
#include "graphics_api/DescriptorHeap.hpp"
#include "graphics_api/resource_type/FreelistBuffer.hpp"
#include "renderer/PrimitiveBatch.hpp"
#include "assets/Asset.hpp"
#include "pipeline/VertexShader.hpp"
#include "pipeline/PixelShader.hpp"
#include "graphics_api/Wrappers/CommandWrapping.hpp"
#include "graphics_api/Wrappers/ResourceWrapping.hpp"
#include "graphics_api/Wrappers/DescriptorWrapping.hpp"
#include "misc/LinearAllocator.hpp"
#include "misc/CallbackExecutor.hpp"
#include "graphics_api/Wrappers/VertexBuffer.hpp"
#include "graphics_api/FreelistBuffer.hpp"

namespace aZero
{
	namespace Asset
	{
		class Mesh;
		class Material;
		class Texture;
	}

	namespace Pipeline
	{
		class PipelineManager;
	}

	class Engine;
	namespace Rendering
	{
		class RenderContext;
		class RenderSurface;

		class Renderer : public NonCopyable
		{
			friend Engine;
			friend RenderContext;
		private:
			// Asset GPU-side data
			struct MeshEntry
			{
				// Indices into the resource descriptor heap for each attribute
				uint32_t PositionsIndex;
				uint32_t UVsIndex;
				uint32_t NormalsIndex;
				uint32_t TangentsIndex;
				uint32_t IndicesIndex;
				uint32_t NumIndices;
			};

			struct MaterialData
			{
				DXM::Vector3 Color = DXM::Vector3::Zero;
				int32_t AlbedoIndex;
				int32_t NormalIndex;
			};

			struct GPUTexture2D
			{
				RenderAPI::Descriptor m_Srv;
				RenderAPI::Texture2D m_Texture;
			};
			//

			// New wrappers and frame context
			struct FrameContext : public NonCopyable
			{
				// todo Remove these and replace with a gpu-only buffer that is owned by the scene, but accessed in the renderer only, and that contains all the scene's primitive data
				//		This buffer should be updated via a compute shader that takes all entity updates done through UpdateRenderState() and copies them to the buffer in vram
				//		So there needs to be a dedicated staging buffer, per primitive type, that the compute shader reads from.
				//		Then there needs to be a dedicated vram-only buffer, per primitive, that the compute shader writes to.
				// todo Create shader resource views for them and access them bindlessly in the shaders
				RenderAPI::Buffer m_StaticMeshBuffer;
				RenderAPI::Descriptor m_StaticMeshDescriptor;

				RenderAPI::Buffer m_PointLightBuffer;
				RenderAPI::Descriptor m_PointLightDescriptor;

				RenderAPI::Buffer m_SpotLightBuffer;
				RenderAPI::Descriptor m_SpotLightDescriptor;

				RenderAPI::Buffer m_DirectionalLightBuffer;
				RenderAPI::Descriptor m_DirectionalLightDescriptor;
				//
				
				// Commandlist stuff
				RenderAPI::CommandListAllocator m_DirectCmdAllocator;
				RenderAPI::CommandList m_DirectCmdList;
				RenderAPI::CommandListAllocator m_CopyCmdAllocator;
				RenderAPI::CommandList m_CopyCmdList;
				RenderAPI::CommandListAllocator m_ComputeCmdAllocator;
				RenderAPI::CommandList m_ComputeCmdList;
				//

				// Per-frame linear staging allocator that stores allocations and can record them at a later point
				LinearAllocator<> m_FrameAllocator;
				struct FrameAllocation
				{
					LinearAllocator<>::Allocation allocation;
					uint32_t dstOffset;
					RenderAPI::Buffer* buffer;
				};
				std::vector<FrameAllocation> m_FrameAllocations;
				RenderAPI::Buffer m_FrameBuffer;

				template<typename T>
				void AddAllocation(const T& data, RenderAPI::Buffer& buffer, uint32_t dstOffset)
				{
					FrameAllocation allocation;
					allocation.allocation = m_FrameAllocator.Allocate(sizeof(T));
					allocation.buffer = &buffer;
					allocation.dstOffset = dstOffset;
					m_FrameAllocations.emplace_back(allocation);
				}

				// !note To be executed before rendering a scene since it might contain asset upload data and other things
				void RecordFrameAllocations(RenderAPI::CommandList& cmdList)
				{
					for (const FrameAllocation& allocation : m_FrameAllocations)
					{
						cmdList->CopyBufferRegion(allocation.buffer->GetResource(), allocation.dstOffset, m_FrameBuffer.GetResource(), allocation.allocation.Offset, allocation.allocation.Size);
					}
				}
				//

				uint64_t m_FrameCompleteSignal; // Latest signal that is related to the context - Should be used to find when the frame context is available again

				// Update with this for each new commandlist that is executed during the frame
				void SetLatestSignal(uint64_t signal)
				{
					m_FrameCompleteSignal = signal;
				}

				// Used when we wanna check if we should render the next frame
				// todo Change this once several different kinds of queues are used
				bool IsReady(RenderAPI::CommandQueue& directQueue)
				{
					// !note Currently stalls cpu if the next frame context isnt ready - Might wanna change it to return early to enable the game-loop to make another lap
					return directQueue.WaitForSignal(m_FrameCompleteSignal, true);
				}

				FrameContext() = default;
				FrameContext(ID3D12Device* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler)
				{
					this->Init(device, resourceHeap, recycler);
				}

				FrameContext(FrameContext&&) noexcept = default;
				FrameContext& operator=(FrameContext&&) noexcept = default;

				void Init(ID3D12Device* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler)
				{
					const uint32_t frameBufferSize = 1000000;
					const RenderAPI::Buffer::Desc frameBufferDesc(frameBufferSize, D3D12_HEAP_TYPE_UPLOAD);
					m_FrameBuffer.Init(device, frameBufferDesc, &recycler);
					m_FrameAllocator.Init(m_FrameBuffer.GetCPUAccessibleMemory(), frameBufferSize);

					const uint32_t primitiveBufferSize = 1000000;
					const RenderAPI::Buffer::Desc primitiveBufferDesc(primitiveBufferSize, D3D12_HEAP_TYPE_UPLOAD);
					m_StaticMeshBuffer.Init(device, primitiveBufferDesc, &recycler);
					m_PointLightBuffer.Init(device, primitiveBufferDesc, &recycler);
					m_SpotLightBuffer.Init(device, primitiveBufferDesc, &recycler);
					m_DirectionalLightBuffer.Init(device, primitiveBufferDesc, &recycler);

					m_DirectCmdAllocator.Init(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
					m_CopyCmdAllocator.Init(device, D3D12_COMMAND_LIST_TYPE_COPY);
					m_ComputeCmdAllocator.Init(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

					m_StaticMeshDescriptor = resourceHeap.CreateDescriptor();
					m_PointLightDescriptor = resourceHeap.CreateDescriptor();
					m_SpotLightDescriptor = resourceHeap.CreateDescriptor();
					m_DirectionalLightDescriptor = resourceHeap.CreateDescriptor();

					m_DirectCmdList = m_DirectCmdAllocator.CreateCommandList();
					m_CopyCmdList = m_CopyCmdAllocator.CreateCommandList();
					m_ComputeCmdList = m_ComputeCmdAllocator.CreateCommandList();
				};

				// Used on engine frame beginning
				void Begin()
				{
					m_FrameAllocator.Reset();
					m_DirectCmdAllocator.FreeCommandBuffer();
					m_CopyCmdAllocator.FreeCommandBuffer();
					m_ComputeCmdAllocator.FreeCommandBuffer();
				}
			};

			uint32_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;

			RenderAPI::ResourceRecycler m_NewResourceRecycler;

			// todo Figure out how this should be used to defer destruction of descriptors so that they wont be used until their no longer in use
			aZero::CallbackExecutor m_CallbackExecutor;

			struct SamplerManager
			{
				enum Type{Anisotropic_8x_Wrap = 0};
				std::vector<RenderAPI::Descriptor> m_Descriptors;

				SamplerManager() = default;
				SamplerManager(ID3D12Device* device, RenderAPI::DescriptorHeap& heap)
				{
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
					m_Descriptors.emplace_back(heap.CreateDescriptor());
					device->CreateSampler(&SamplerDesc, m_Descriptors.at(m_Descriptors.size() - 1).GetCpuHandle());
				}

				const RenderAPI::Descriptor& GetSampler(Type type) { return m_Descriptors.at(type); }
			};

			RenderAPI::CommandQueue m_DirectCommandQueue;
			RenderAPI::CommandQueue m_CopyCommandQueue;
			RenderAPI::CommandQueue m_ComputeCommandQueue;

			SamplerManager m_SamplerManager;
			RenderAPI::DescriptorHeap m_ResourceHeapNew;
			RenderAPI::DescriptorHeap m_SamplerHeapNew;
			RenderAPI::DescriptorHeap m_RTVHeapNew;
			RenderAPI::DescriptorHeap m_DSVHeapNew;

			std::vector<FrameContext> m_FrameContexts;
			FrameContext& GetCurrentContext() 
			{ 
				return m_FrameContexts.at(m_FrameIndex); 
			}

			bool AdvanceFrameIfReady()
			{
				const uint32_t newPotentialFrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());

				if (!m_FrameContexts.at(newPotentialFrameIndex).IsReady(m_DirectCommandQueue))
				{
					return false;
				}

				this->GetCurrentContext().Begin();

				return true;
			}

			void NewInit(ID3D12Device* device, uint32_t numFramesInFlight)
			{
				m_DirectCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
				m_CopyCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY);
				m_ComputeCommandQueue = RenderAPI::CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

				m_ResourceHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, true);
				m_SamplerHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20, true);
				m_RTVHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100, false);
				m_DSVHeapNew = RenderAPI::DescriptorHeap(device, m_CallbackExecutor, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100, false);
				// noah
				for (int32_t i = 0; i < numFramesInFlight; i++)
				{
					m_FrameContexts.emplace_back(device, m_ResourceHeapNew, m_NewResourceRecycler);
				}

				m_SamplerManager = SamplerManager(device, m_SamplerHeapNew);

				m_MeshEntryBuffer = RenderAPI::NewFreelistBuffer<MeshEntry>(device, 1000, &m_NewResourceRecycler);
				m_NewMaterialBuffer = RenderAPI::NewFreelistBuffer<MaterialData>(device, 1000, &m_NewResourceRecycler);
			}

			bool NewBeginFrame()
			{
				const bool hasNewFrameStarted = AdvanceFrameIfReady();
				if (hasNewFrameStarted)
				{
					m_FrameIndex = static_cast<uint32_t>(m_FrameCount % m_FrameContexts.size());
					m_FrameCount++;
				}

				// todo Maybe do something specific if a new frame cannot begin?

				return hasNewFrameStarted;
			}

			void NewRender()
			{
				// todo Check if it can render if rtv etc. is free... Maybe do the check after performing the queued updates?

				FrameContext& frameContext = this->GetCurrentContext();
				// Perform uploads for all updated/new assets and other stagings
				frameContext.RecordFrameAllocations(frameContext.m_DirectCmdList);
				m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);

				// todo Do rendering
			}

			void CopyTextureToTexture(RenderAPI::Texture2D& dstTexture, RenderAPI::Texture2D& srcTexture)
			{
				FrameContext& frameContext = this->GetCurrentContext();

				ID3D12Resource* dstResource = dstTexture.GetResource();
				ID3D12Resource* srcResource = srcTexture.GetResource();

				std::vector<RenderAPI::ResourceTransitionBundles> preCopyBarriers;
				preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, dstResource });
				preCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE, srcResource });
				
				// todo Change to this once enhanced barriers is added
				//dstTexture.CreateTransition()
				RenderAPI::TransitionResources(frameContext.m_DirectCmdList, preCopyBarriers);

				frameContext.m_DirectCmdList->CopyResource(dstTexture.GetResource(), srcTexture.GetResource());

				std::vector<RenderAPI::ResourceTransitionBundles> postCopyBarriers;
				postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, dstResource });
				postCopyBarriers.push_back({ D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON, srcResource });

				// todo Change to this once enhanced barriers is added
				//dstTexture.CreateTransition()
				RenderAPI::TransitionResources(frameContext.m_DirectCmdList, postCopyBarriers);

				m_DirectCommandQueue.ExecuteCommandList(frameContext.m_DirectCmdList);
			}

			void FlushGPU() 
			{ 
				m_DirectCommandQueue.Flush();

				// todo When we're also using other types of queues we need to add them here and do some other stuff
			}

			ID3D12Device* GetDevice() const
			{
				ID3D12Device* device;
				const HRESULT deviceRes = m_DirectCommandQueue.Get()->GetDevice(IID_PPV_ARGS(&device));
				if (FAILED(deviceRes))
				{
					throw std::invalid_argument("Failed to get device of renderer");
				}
				return device;
			}

			RenderAPI::NewFreelistBuffer<MaterialData> m_NewMaterialBuffer;
			std::unordered_map<Asset::AssetID, uint32_t> m_NewMaterialAllocMap;

			RenderAPI::NewFreelistBuffer<MeshEntry> m_MeshEntryBuffer;
			std::unordered_map<Asset::AssetID, uint32_t> m_NewMeshEntryAllocMap;

			std::unordered_map<Asset::AssetID, RenderAPI::VertexBuffer> m_GPUMeshes;
			std::unordered_map<Asset::AssetID, GPUTexture2D> m_GPUTextures;

			Pipeline::VertexShader m_NewBasePassVS;
			Pipeline::PixelShader m_NewBasePassPS;

			//---------------------------------------------------------------------------------------------------------------//
			// todo Remove once the new renderer pipeline is completed

			struct MaterialHandle
			{
				uint32_t Index; // Index into the material structured buffer
			};

			struct MeshBuffers
			{
				D3D12::FreelistBuffer m_PositionBuffer;
				D3D12::FreelistBuffer m_UVBuffer;
				D3D12::FreelistBuffer m_NormalBuffer;
				D3D12::FreelistBuffer m_TangentBuffer;
				D3D12::FreelistBuffer m_IndexBuffer;
				D3D12::FreelistBuffer m_MeshEntryBuffer;
				std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_PositionAllocMap;
				std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_UVAllocMap;
				std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_NormalAllocMap;
				std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_TangentAllocMap;
				std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_IndexAllocMap;
				std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_MeshEntryAllocMap;
			};
			MeshBuffers m_MeshBuffers;
			D3D12::FreelistBuffer m_MaterialBuffer;
			std::unordered_map<Asset::AssetID, DS::FreelistAllocator::AllocationHandle> m_MaterialAllocMap;

			void AllocateFreelistMesh(Asset::Mesh& mesh, ID3D12GraphicsCommandList* cmdList);
			void InitCommandRecording();
			void InitFrameResources();
			void InitDescriptorHeaps();
			void InitSamplers();
			void UploadStagedAssets();

			ID3D12Device* m_diDevice = nullptr;

			D3D12::CommandQueue m_GraphicsQueue;

			D3D12::DescriptorHeap m_ResourceHeap;
			D3D12::DescriptorHeap m_SamplerHeap;
			D3D12::DescriptorHeap m_RTVHeap;
			D3D12::DescriptorHeap m_DSVHeap;

			D3D12::Descriptor m_AnisotropicSampler;

			// todo Remove once the render graph is completed
			RenderGraphPass m_DefaultRenderPass;
			Pipeline::VertexShader m_BasePassVS;
			Pipeline::PixelShader m_BasePassPS;

			// todo Remove once frame resources are reworked
			D3D12::ResourceRecycler m_ResourceRecycler;
			std::vector<std::shared_ptr<D3D12::GPUBuffer>> m_StaticMeshFrameBuffers;
			std::vector<std::shared_ptr<D3D12::GPUBuffer>> m_PointLightFrameBuffers;
			std::vector<std::shared_ptr<D3D12::GPUBuffer>> m_SpotLightFrameBuffers;
			std::vector<std::shared_ptr<D3D12::GPUBuffer>> m_DirectionalLightFrameBuffers;
			D3D12::CommandContextAllocator m_CommandContextAllocator;

			uint32_t m_BufferCount;

			// Replace with the new resource class and linearallocator combo
			std::vector<D3D12::LinearFrameAllocator> m_AssetStagingAllocators;

			struct TextureRenderAsset
			{
				D3D12::ShaderResourceView m_Srv;
				D3D12::GPUTexture m_Resource;
			};
			std::unordered_map<Asset::AssetID, TextureRenderAsset> m_TextureAllocMap;

			// Used to know where a mesh is located in the buffers
			struct MeshHandle
			{
				uint32_t StartIndex; // Index to the first vertex/index for the mesh in the buffers. This is used to know where to start fetching the vertices from in the vs
				uint32_t NumVertices; // Number of vertices/indices that the mesh has. This is used for the draw call
			};

			//---------------------------------------------------------------------------------------------------------------//
		public:

			void NewUpdateRenderState(Asset::NewMesh* mesh);
			void NewUpdateRenderState(Asset::NewMaterial* material);
			void NewUpdateRenderState(Asset::NewTexture* texture);
			void NewRemoveRenderState(Asset::NewMesh* mesh);
			void NewRemoveRenderState(Asset::NewMaterial* material);
			void NewRemoveRenderState(Asset::NewTexture* texture);

			Renderer() = default;

			Renderer(ID3D12Device* device, uint32_t bufferCount);

			Renderer(Renderer&&) noexcept = default;
			Renderer& operator=(Renderer&&) noexcept = default;

			//---------------------------------------------------------------------------------------------------------------//
			// todo Remove once the new renderer pipeline is completed
			Pipeline::ScenePass* scenePass;

			// Marks the beginning of a frame.
			void BeginFrame();

			// Copies the rendered surface to the swapchain.
			// todo Impl up/downscaling to match the SrcTexture to the DstTexture (back buffer)
			void CopyTextureToTexture(ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture);

			// todo Change the logic of this so it syncs on a circular buffer once frame resources have been reworked
			void EndFrame();

			// Renders a scene to the target surfaces.
			void Render(Scene::Scene& Scene, Rendering::RenderSurface& RenderSurface, bool ClearRenderSurface, Rendering::RenderSurface& DepthSurface, bool ClearDepthSurface);

			// Stalls the CPU until all GPU work is finished
			void FlushGraphicsQueue() { m_GraphicsQueue.FlushImmediate(); }

			void UpdateRenderState(Asset::AssetHandle<Asset::Mesh>& mesh);
			void UpdateRenderState(Asset::AssetHandle<Asset::Material>& material);
			void UpdateRenderState(Asset::AssetHandle<Asset::Texture>& texture);
			void RemoveRenderState(Asset::AssetHandle<Asset::Mesh>& mesh);
			void RemoveRenderState(Asset::AssetHandle<Asset::Material>& material);
			void RemoveRenderState(Asset::AssetHandle<Asset::Texture>& texture);
			//---------------------------------------------------------------------------------------------------------------//
		};

	}
}