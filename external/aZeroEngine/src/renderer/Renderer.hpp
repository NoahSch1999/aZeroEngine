#pragma once
#include "graphics_api/CommandQueue.hpp"
#include "renderer/render_graph/RenderGraph.hpp"
#include "ecs/aZeroECS.hpp"
#include "window/RenderWindow.hpp"
#include "scene/Scene.hpp"
#include "graphics_api/DescriptorHeap.hpp"
#include "graphics_api/resource_type/FreelistBuffer.hpp"
#include "renderer/render_asset/RenderAssetManager.hpp"
#include "renderer/PrimitiveBatch.hpp"

#include <span>

namespace aZero
{
	class Engine;
	namespace Rendering
	{
		class RenderInterface;

		class Renderer
		{
			friend Engine;
			friend RenderInterface;

		private:
			ID3D12Device* m_Device;

			// TODO: Maybe move our compiler outside of the renderer to engine or its own "module"?
			CComPtr<IDxcCompiler3> m_Compiler;

			D3D12::CommandQueue m_GraphicsQueue;
			uint32_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;

			D3D12::ResourceRecycler m_ResourceRecycler;

			D3D12::FreelistBuffer m_VertexBuffer;
			D3D12::FreelistBuffer m_IndexBuffer;
			D3D12::FreelistBuffer m_MeshEntryBuffer;
			D3D12::FreelistBuffer m_MaterialBuffer;

			D3D12::DescriptorHeap m_ResourceHeap;
			D3D12::DescriptorHeap m_SamplerHeap;
			D3D12::DescriptorHeap m_RTVHeap;
			D3D12::DescriptorHeap m_DSVHeap;

			std::unique_ptr<Asset::RenderAssetManager> m_AssetManager;

			uint32_t m_BufferCount;

			// TODO: Remove once we have a more sophisticated system
			std::string m_ContentPath;
			std::vector<D3D12::LinearFrameAllocator> m_FrameAllocators;
			D3D12::CommandContext m_GraphicsCommandContext;
			D3D12::CommandContext m_PackedGPULookupBufferUpdateContext;
			D3D12::Descriptor m_DefaultSamplerDescriptor;

			D3D12::GPUTexture m_FinalRenderSurface;
			D3D12::Descriptor m_FinalRenderSurfaceUAV;
			D3D12::RenderTargetView m_FinalRenderSurfaceRTV;

			D3D12::GPUTexture m_SceneDepthTexture;
			D3D12::DepthStencilView m_SceneDepthTextureDSV;

			DXM::Vector2 m_RenderResolution;
			D3D12_CLEAR_VALUE m_RTVClearColor;
			D3D12_CLEAR_VALUE m_DSVClearColor;

			D3D12::RenderPass Pass;

			// TODO: Change to LinearBuffer
			D3D12::GPUBuffer m_BatchVertexBuffer;

			D3D12::RenderPass m_BatchPassDepthP;
			D3D12::RenderPass m_BatchPassNoDepthP;

			D3D12::RenderPass m_BatchPassDepthL;
			D3D12::RenderPass m_BatchPassNoDepthL;

			D3D12::RenderPass m_BatchPassDepthT;
			D3D12::RenderPass m_BatchPassNoDepthT;
			//

			RenderGraphPass m_StaticMeshPass;
			RenderGraph m_RenderGraph;
			std::set<std::shared_ptr<Window::RenderWindow>> m_FrameWindows;

		private:

			D3D12::LinearFrameAllocator& GetFrameAllocator()
			{
				return m_FrameAllocators[m_FrameIndex];
			}

			void SetBufferCount(uint32_t BufferCount)
			{
				if (BufferCount <= 1)
				{
					throw std::runtime_error("Renderer::Init => Invalid BufferCount");
				}
				m_BufferCount = BufferCount;
			}

			void SetupRenderPipeline();

			void RecordAssetManagerCommands();

			void InitPrimitiveBatchPipeline();

			void PrepareGraphForScene(Scene::Scene& InScene);

			void RenderPrimitiveBatch(const PrimitiveBatch& Batch, const Scene::Scene::Camera& Camera);

			void GetRelevantStaticMeshes(
				Scene::Scene& Scene,
				const DirectX::BoundingFrustum& Frustum,
				const DXM::Matrix& ViewMatrix,
				StaticMeshBatches& OutStaticMeshBatches);

			void RenderStaticMeshes(
				Scene::Scene& InScene,
				const Scene::Scene::Camera& Camera,
				const StaticMeshBatches& StaticMeshBatches,
				uint32_t NumDirectionalLights,
				uint32_t NumPointLights,
				uint32_t NumSpotLights
			);

			// TODO: Impl
			void GetRelevantSkeletalMeshes(Scene::Scene& Scene, const Scene::Scene::Camera& Camera) { }

			// TODO: Impl
			void RenderSkeletalMeshes(Scene::Scene& InScene, const Scene::Scene::Camera& Camera) { }

			void GetDirectionalLights(Scene::Scene& Scene,
				uint32_t& NumDirectionalLights);

			void GetRelevantPointLights(Scene::Scene& Scene,
				const DirectX::BoundingFrustum& Frustum,
				const DXM::Matrix& ViewMatrix,
				uint32_t& NumPointLights);

			void GetRelevantSpotLights(Scene::Scene& Scene,
				const DirectX::BoundingFrustum& Frustum,
				const DXM::Matrix& ViewMatrix,
				uint32_t& NumSpotLights);

			void ClearRenderSurfaces();

			void CopyRenderSurfaceToTexture(ID3D12Resource* TargetTexture);

		protected:

			void BeginFrame();

			void Render(Scene::Scene& Scene, const std::vector<PrimitiveBatch*>& Batches, std::shared_ptr<Window::RenderWindow> Window);

			void EndFrame();

			void FlushImmediate() { m_GraphicsQueue.FlushImmediate(); }

		public:
			Renderer(ID3D12Device* Device, const DXM::Vector2& RenderResolution, uint32_t BufferCount, const std::string& ContentPath)
			{
				this->Init(Device, RenderResolution, BufferCount, ContentPath);
			}

			void Init(ID3D12Device* Device, const DXM::Vector2& RenderResolution, uint32_t BufferCount, const std::string& ContentPath);

			~Renderer()
			{
				m_GraphicsQueue.FlushCommands();
			}

			// TODO: Impl
			void ChangeRenderResolution(const DXM::Vector2& Dimensions)
			{

			}

			IDxcCompiler3& GetCompiler() { return *m_Compiler.p; }

			void MarkRenderStateDirty(const std::shared_ptr<Asset::Mesh>& MeshAsset)
			{
				if (!m_AssetManager->HasRenderHandle(*MeshAsset.get()))
				{
					D3D12::LinearFrameAllocator& Allocator = this->GetFrameAllocator();

					const Asset::MeshData& Data = MeshAsset->GetData();
					DS::FreelistAllocator::AllocationHandle NewVBHandle;
					DS::FreelistAllocator::AllocationHandle NewIBHandle;
					DS::FreelistAllocator::AllocationHandle NewMeshEntryHandle;

					m_VertexBuffer.Allocate(
						NewVBHandle,
						static_cast<uint32_t>(Data.Vertices.size()) * sizeof(decltype(Data.Vertices.at(0))));

					ID3D12GraphicsCommandList* CmdList = m_GraphicsCommandContext.GetCommandList();

					m_VertexBuffer.Write(CmdList, Allocator, NewVBHandle, (void*)Data.Vertices.data());

					m_IndexBuffer.Allocate(
						NewIBHandle,
						static_cast<uint32_t>(Data.Indices.size()) * sizeof(decltype(Data.Indices.at(0))));

					m_IndexBuffer.Write(CmdList, Allocator, NewIBHandle, (void*)Data.Indices.data());

					m_MeshEntryBuffer.Allocate(
						NewMeshEntryHandle,
						sizeof(Asset::MeshEntry::GPUData)
					);

					Asset::MeshEntry::GPUData NewMeshEntryGPUData;
					NewMeshEntryGPUData.VertexStartOffset = NewVBHandle.GetStartOffset() / sizeof(Asset::VertexData);
					NewMeshEntryGPUData.IndexStartOffset = NewIBHandle.GetStartOffset() / sizeof(std::uint32_t);
					NewMeshEntryGPUData.NumIndices = Data.Indices.size();
					m_MeshEntryBuffer.Write(CmdList, Allocator, NewMeshEntryHandle, (void*)&NewMeshEntryGPUData);

					Asset::MeshEntry NewMeshEntry;
					NewMeshEntry.VertexBufferAllocHandle = std::move(NewVBHandle);
					NewMeshEntry.IndexBufferAllocHandle = std::move(NewIBHandle);
					NewMeshEntry.MeshEntryAllocHandle = std::move(NewMeshEntryHandle);

					m_AssetManager->AddRenderHandle(*MeshAsset.get(), std::move(NewMeshEntry));

					m_GraphicsCommandContext.StopRecording();
					m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
				}
				else
				{
					// TODO: Impl
				}
			}

			void MarkRenderStateDirty(const std::shared_ptr<Asset::Texture>& TextureAsset)
			{
				if (!m_AssetManager->HasRenderHandle(*TextureAsset.get()))
				{
					const Asset::TextureData& Data = TextureAsset->GetData();
					Asset::TextureEntry NewEntry;

					NewEntry.Texture.Init(
						m_Device,
						{ Data.m_Dimensions.x, Data.m_Dimensions.y, 1 },
						Data.m_Format,
						D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
						m_ResourceRecycler,
						1, // TODO: Mip should be in the m_AssetData cache
						D3D12_RESOURCE_STATE_COMMON,
						std::optional<D3D12_CLEAR_VALUE>{}
					);

					const uint64_t StagingBufferSize = static_cast<uint64_t>(GetRequiredIntermediateSize(NewEntry.Texture.GetResource(), 0, 1));
					D3D12::GPUBuffer StagingBuffer;
					StagingBuffer.Init(m_Device, D3D12_HEAP_TYPE_UPLOAD, StagingBufferSize, m_ResourceRecycler);

					std::vector<D3D12::ResourceTransitionBundles> Bundle;
					Bundle.push_back({});
					Bundle.at(0).ResourcePtr = NewEntry.Texture.GetResource();
					Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COMMON;
					Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
					D3D12::TransitionResources(m_GraphicsCommandContext.GetCommandList(), Bundle);

					D3D12_SUBRESOURCE_DATA SubresourceData{};
					SubresourceData.pData = Data.m_Data.data();
					SubresourceData.RowPitch = /*roundUp(*/Data.m_Dimensions.x * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
					SubresourceData.SlicePitch = SubresourceData.RowPitch * Data.m_Dimensions.y;

					UpdateSubresources(
						m_GraphicsCommandContext.GetCommandList(),
						NewEntry.Texture.GetResource(),
						StagingBuffer.GetResource(),
						0, 0, 1, &SubresourceData);

					Bundle.at(0).ResourcePtr = NewEntry.Texture.GetResource();
					Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
					Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					D3D12::TransitionResources(m_GraphicsCommandContext.GetCommandList(), Bundle);

					m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);

					D3D12::Descriptor Descriptor;
					m_ResourceHeap.GetDescriptor(Descriptor);
					NewEntry.SRV.Init(m_Device, Descriptor, NewEntry.Texture, NewEntry.Texture.GetResource()->GetDesc().Format, 1, 0, 0, 0);

					m_AssetManager->AddRenderHandle(*TextureAsset.get(), std::move(NewEntry));
				}
				else
				{
					// TODO: Impl
				}
			}

			void MarkRenderStateDirty(const std::shared_ptr<Asset::Material>& MaterialAsset)
			{
				const Asset::RenderAssetID ID = MaterialAsset->GetRenderID();
				if (!m_AssetManager->GetRenderHandle(*MaterialAsset.get()))
				{
					Asset::MaterialEntry NewEntry;
					m_MaterialBuffer.Allocate(NewEntry.MaterialAllocHandle, sizeof(Asset::MaterialData::MaterialRenderData));
					m_AssetManager->AddRenderHandle(*MaterialAsset.get(), std::move(NewEntry));
				}

				const Asset::MaterialData& Data = MaterialAsset->GetData();
				Asset::MaterialData::MaterialRenderData RenderData;
				RenderData.m_Color = Data.m_Color;
				if (Data.m_AlbedoTexture && m_AssetManager->HasRenderHandle(*Data.m_AlbedoTexture.get()))
				{
					RenderData.m_AlbedoDescriptorIndex = m_AssetManager->GetRenderHandle(*Data.m_AlbedoTexture.get()).value()->SRV.GetDescriptorIndex();
				}
				else
				{
					RenderData.m_AlbedoDescriptorIndex = -1;
				}

				if (Data.m_NormalMap && m_AssetManager->HasRenderHandle(*Data.m_NormalMap.get()))
				{
					RenderData.m_NormalMapDescriptorIndex = m_AssetManager->GetRenderHandle(*Data.m_NormalMap.get()).value()->SRV.GetDescriptorIndex();
				}
				else
				{
					RenderData.m_NormalMapDescriptorIndex = -1;
				}

				ID3D12GraphicsCommandList* const CmdList = m_GraphicsCommandContext.GetCommandList();
				m_MaterialBuffer.Write(CmdList, this->GetFrameAllocator(), m_AssetManager->GetRenderHandle(*MaterialAsset.get()).value()->MaterialAllocHandle, &RenderData);
				m_GraphicsCommandContext.StopRecording();
				m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);
			}
		};
	}
}