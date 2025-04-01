#pragma once
#include "graphics_api/CommandQueue.hpp"
#include "renderer/render_graph/RenderGraph.hpp"
#include "ecs/aZeroECS.hpp"
#include "scene/Scene.hpp"
#include "graphics_api/DescriptorHeap.hpp"
#include "graphics_api/resource_type/FreelistBuffer.hpp"
#include "renderer/render_asset/RenderAssetManager.hpp"
#include "renderer/PrimitiveBatch.hpp"

namespace aZero
{
	class Engine;
	namespace Rendering
	{
		class RenderContext;

		class Renderer
		{
			friend Engine;
			friend RenderContext;

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
			D3D12::Descriptor m_DefaultSamplerDescriptor;

			D3D12::CommandContextAllocator m_GraphicsCmdContextAllocator;

			DXM::Vector2 m_RenderResolution;
			D3D12_CLEAR_VALUE m_DSVClearColor;

			D3D12::LinearBuffer m_BatchVertexBuffer;

			D3D12::RenderPass m_BatchPassDepthP;
			D3D12::RenderPass m_BatchPassNoDepthP;

			D3D12::RenderPass m_BatchPassDepthL;
			D3D12::RenderPass m_BatchPassNoDepthL;

			D3D12::RenderPass m_BatchPassDepthT;
			D3D12::RenderPass m_BatchPassNoDepthT;

			RenderGraphPass m_StaticMeshPass;
			//

			RenderGraph m_RenderGraph;

		private:

			D3D12::LinearFrameAllocator& GetFrameAllocator()
			{
				return m_FrameAllocators[m_FrameIndex];
			}

			void SetupRenderPipeline();

			void InitPrimitiveBatchPipeline();

			void RenderPrimitiveBatch(D3D12::CommandContext& CmdContext, const D3D12::RenderTargetView& ColorView, const PrimitiveBatch& Batch, const Scene::Scene::Camera& Camera);

			void CopyRenderSurfaceToTexture(D3D12::CommandContext& CmdContext, ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture);

			void SetupRenderGraph()
			{
				m_RenderGraph.Init(&m_ResourceHeap, &m_SamplerHeap);

				D3D12::RenderPass Pass;
				{
					D3D12::Shader BasePassVS;
					BasePassVS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "BasePass.vs.hlsl");

					D3D12::Shader BasePassPS;
					BasePassPS.CompileFromFile(this->GetCompiler(), m_ContentPath + SHADER_SOURCE_RELATIVE_PATH + "BasePass.ps.hlsl");

					Pass.Init(m_Device, BasePassVS, BasePassPS, { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, DXGI_FORMAT_D24_UNORM_S8_UINT);
				}
				m_StaticMeshPass = RenderGraphPass(std::move(Pass));
				m_StaticMeshPass.BindBuffer("VertexBuffer", D3D12::SHADER_TYPE::VS, &m_VertexBuffer.GetBuffer());
				m_StaticMeshPass.BindBuffer("IndexBuffer", D3D12::SHADER_TYPE::VS, &m_IndexBuffer.GetBuffer());
				m_StaticMeshPass.BindBuffer("MeshEntries", D3D12::SHADER_TYPE::VS, &m_MeshEntryBuffer.GetBuffer());
				m_StaticMeshPass.BindBuffer("MaterialBuffer", D3D12::SHADER_TYPE::PS, &m_MaterialBuffer.GetBuffer());
				m_StaticMeshPass.m_ShouldClearOnExecute = false;

				m_RenderGraph.AddPass(m_StaticMeshPass, -1);
			}

			void BindGeometryPassBuffers(Scene::Scene& InScene)
			{
				m_StaticMeshPass.BindBuffer("InstanceBuffer", D3D12::SHADER_TYPE::VS,
					&InScene.m_RenderProxy.GetObjects<Scene::Scene::StaticMesh>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

				m_StaticMeshPass.BindBuffer("DirectionalLightBuffer", D3D12::SHADER_TYPE::PS,
					&InScene.m_RenderProxy.GetObjects<DirectionalLightData>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

				m_StaticMeshPass.BindBuffer("PointLightBuffer", D3D12::SHADER_TYPE::PS,
					&InScene.m_RenderProxy.GetObjects<PointLightData>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

				m_StaticMeshPass.BindBuffer("SpotLightBuffer", D3D12::SHADER_TYPE::PS,
					&InScene.m_RenderProxy.GetObjects<SpotLightData>().m_RenderBuffers.at(m_FrameIndex).GetBuffer());

				struct PixelShaderConstants
				{
					uint32_t SamplerIndex;
				}PSConstants;

				PSConstants.SamplerIndex = m_DefaultSamplerDescriptor.GetHeapIndex();
				m_StaticMeshPass.BindConstant("PixelShaderConstantsBuffer", D3D12::SHADER_TYPE::PS, &PSConstants, sizeof(PixelShaderConstants));
			}

			void RecordFrameAllocatorCommands(D3D12::CommandContext& CmdContext)
			{
				D3D12::LinearFrameAllocator& FrameAllocator = m_FrameAllocators[m_FrameIndex];
				FrameAllocator.RecordAllocations(CmdContext.GetCommandList());
				CmdContext.StopRecording();
				m_GraphicsQueue.ExecuteContext(CmdContext);
			}

			void ClearRenderTarget(const D3D12::RenderTargetView& RenderTarget, D3D12::CommandContext& CmdContext)
			{
				CmdContext.GetCommandList()->ClearRenderTargetView(RenderTarget.GetDescriptorHandle(), RenderTarget.GetClearValue().Color, 0, NULL);
			}

			void ClearDepthSurface(const D3D12::DepthStencilView& DepthStencilTarget, D3D12::CommandContext& CmdContext)
			{
				CmdContext.GetCommandList()->ClearDepthStencilView(
					DepthStencilTarget.GetDescriptorHandle(),
					D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
					DepthStencilTarget.GetClearValue().DepthStencil.Depth, DepthStencilTarget.GetClearValue().DepthStencil.Stencil,
					0, NULL);
			}

			uint32_t UploadDirectionalLightData(Scene::Scene& Scene, D3D12::CommandContext& CmdContext)
			{
				// Allocate multiple buffers based on what buffering count the renderer works with
				std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<DirectionalLightData>().m_RenderBuffers;
				if (RenderBuffers.size() != m_BufferCount)
				{
					RenderBuffers.resize(m_BufferCount);
				}

				// Initialize buffers for the light data if the scene hasn't been rendered before
				D3D12::LinearBuffer& DirLightBuffer = RenderBuffers.at(m_FrameIndex);
				DirLightBuffer.Reset();
				if (!DirLightBuffer.IsInitalized())
				{
					DirLightBuffer.Init(m_Device,
						D3D12_HEAP_TYPE_UPLOAD,
						sizeof(DirectionalLightData) * std::max(static_cast<size_t>(1), Scene.GetObjects<DirectionalLightData>().size()),
						m_ResourceRecycler
					);
				}

				// Upload data
				for (const auto& LightData : Scene.GetObjects<DirectionalLightData>())
				{
					DirLightBuffer.Write(CmdContext.GetCommandList(), (void*)&LightData, sizeof(DirectionalLightData));
				}

				// Cache number of lights to know the max to iterate over in shader
				return DirLightBuffer.GetOffset() / sizeof(DirectionalLightData);
			}

			uint32_t UploadPointLightData(const DirectX::BoundingFrustum& Frustum, Scene::Scene& Scene, D3D12::CommandContext& CmdContext)
			{
				std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<PointLightData>().m_RenderBuffers;
				if (RenderBuffers.size() != m_BufferCount)
				{
					RenderBuffers.resize(m_BufferCount);
				}

				D3D12::LinearBuffer& PointLightBuffer = RenderBuffers.at(m_FrameIndex);
				PointLightBuffer.Reset();
				if (!PointLightBuffer.IsInitalized())
				{
					PointLightBuffer.Init(m_Device,
						D3D12_HEAP_TYPE_UPLOAD,
						sizeof(PointLightData) * std::max(static_cast<size_t>(1), Scene.GetObjects<PointLightData>().size()),
						m_ResourceRecycler
					);
				}

				for (const auto& LightData : Scene.GetObjects<PointLightData>())
				{
					// TODO: Add culling
					/*const DirectX::BoundingSphere Sphere(DXM::Vector3::Transform(PointLight.Position, ViewMatrix), PointLight.Range);
					if (Frustum.Intersects(Sphere))
					{*/
					PointLightBuffer.Write(CmdContext.GetCommandList(), (void*)&LightData, sizeof(PointLightData));
					//}
				}

				return PointLightBuffer.GetOffset() / sizeof(PointLightData);
			}

			uint32_t UploadSpotLightData(const DirectX::BoundingFrustum& Frustum, Scene::Scene& Scene, D3D12::CommandContext& CmdContext)
			{
				std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<SpotLightData>().m_RenderBuffers;
				if (RenderBuffers.size() != m_BufferCount)
				{
					RenderBuffers.resize(m_BufferCount);
				}

				D3D12::LinearBuffer& SpotLightBuffer = RenderBuffers.at(m_FrameIndex);
				SpotLightBuffer.Reset();
				if (!SpotLightBuffer.IsInitalized())
				{
					SpotLightBuffer.Init(m_Device,
						D3D12_HEAP_TYPE_UPLOAD,
						sizeof(SpotLightData) * std::max(static_cast<size_t>(1), Scene.GetObjects<SpotLightData>().size()),
						m_ResourceRecycler
					);
				}

				ID3D12GraphicsCommandList* CmdList = CmdContext.GetCommandList();

				for (const auto& LightData : Scene.GetObjects<SpotLightData>())
				{
					// TODO: Add culling
					SpotLightBuffer.Write(CmdList, (void*)&LightData, sizeof(SpotLightData));
				}

				return SpotLightBuffer.GetOffset() / sizeof(SpotLightData);
			}

			void UploadStaticMeshData(StaticMeshBatches& OutStaticMeshBatches, const DirectX::BoundingFrustum& Frustum, const DXM::Matrix& ViewMatrix, Scene::Scene& Scene, D3D12::CommandContext& CmdContext)
			{
				for (const auto& StaticMesh : Scene.GetObjects<Scene::Scene::StaticMesh>())
				{
					const DirectX::BoundingSphere Sphere(DXM::Vector3::Transform(StaticMesh.DXBounds.Center, ViewMatrix), StaticMesh.DXBounds.Radius);
					if (Frustum.Intersects(Sphere))
					{
						uint32_t MeshIndex;
						uint32_t MaterialIndex;
						if (m_AssetManager->HasRenderHandle(*StaticMesh.Mesh.get()))
						{
							MeshIndex = m_AssetManager->GetRenderHandle(*StaticMesh.Mesh.get()).value()->MeshEntryAllocHandle.GetStartOffset() / sizeof(Asset::MeshEntry::GPUData);
						}
						else
						{
							continue;
						}

						if (m_AssetManager->HasRenderHandle(*StaticMesh.Material.get()))
						{
							MaterialIndex = m_AssetManager->GetRenderHandle(*StaticMesh.Material.get()).value()->MaterialAllocHandle.GetStartOffset() / sizeof(Asset::MaterialData::MaterialRenderData);
						}
						else
						{
							continue;
						}

						auto& Batch = OutStaticMeshBatches.Batches[MaterialIndex][MeshIndex];
						Batch.NumVertices = StaticMesh.Mesh->GetData().Indices.size();

						StaticMeshBatches::PerInstanceData InstanceData;
						InstanceData.WorldMatrix = StaticMesh.WorldMatrix;
						Batch.InstanceData.emplace_back(std::move(InstanceData));
					}
				}

				std::vector<D3D12::LinearBuffer>& RenderBuffers = Scene.m_RenderProxy.GetObjects<Scene::Scene::StaticMesh>().m_RenderBuffers;
				if (RenderBuffers.size() != m_BufferCount)
				{
					RenderBuffers.resize(m_BufferCount);
				}

				D3D12::LinearBuffer& InstanceBuffer = RenderBuffers.at(m_FrameIndex);
				InstanceBuffer.Reset();
				if (!InstanceBuffer.IsInitalized())
				{
					InstanceBuffer.Init(m_Device,
						D3D12_HEAP_TYPE_UPLOAD,
						sizeof(StaticMeshBatches::PerInstanceData) * std::max(static_cast<size_t>(1), Scene.GetObjects<Scene::Scene::StaticMesh>().size()),
						m_ResourceRecycler
					);
				}

				// TODO: Try making this purely vectors
				uint32_t InstanceStartOffset = 0;
				for (auto& [MaterialIndex, MeshToBatchMap] : OutStaticMeshBatches.Batches)
				{
					for (auto& [MeshIndex, Batch] : MeshToBatchMap)
					{
						Batch.StartInstanceOffset = InstanceStartOffset;
						for (auto& InstanceData : Batch.InstanceData)
						{
							InstanceBuffer.Write(CmdContext.GetCommandList(), (void*)&InstanceData, sizeof(StaticMeshBatches::PerInstanceData));
						}
						InstanceStartOffset += Batch.InstanceData.size();
					}
				}
			}

			void BeginFrame();

		protected:

			// Public rendering commands accessible via RenderContext
			void Render(Scene::Scene& Scene, const D3D12::RenderTargetView& RenderSurface, bool ClearRenderSurface, const D3D12::DepthStencilView& DepthSurface, bool ClearDepthSurface)
			{
				auto CmdContextHandle = m_GraphicsCmdContextAllocator.GetContext();
				if (!CmdContextHandle.has_value())
				{
					throw std::runtime_error("No more command contexts");
				}

				D3D12::CommandContext& CmdContext = *CmdContextHandle->m_Context;

				this->RecordFrameAllocatorCommands(CmdContext);

				if (ClearRenderSurface)
				{
					this->ClearRenderTarget(RenderSurface, CmdContext);
				}

				if (ClearDepthSurface)
				{
					this->ClearDepthSurface(DepthSurface, CmdContext);
				}

				const uint32_t NumDirectionalLights = UploadDirectionalLightData(Scene, CmdContext);

				m_StaticMeshPass.BindRenderTarget(0, &RenderSurface);
				m_StaticMeshPass.BindDepthStencil(&DepthSurface);

				for (auto& Camera : Scene.GetObjects<Scene::Scene::Camera>())
				{
					if (Camera.IsActive)
					{
						const DirectX::BoundingFrustum Frustum = DirectX::BoundingFrustum(Camera.ProjMatrix, true);

						const uint32_t NumPointLights = UploadPointLightData(Frustum, Scene, CmdContext);

						const uint32_t NumSpotLights = UploadSpotLightData(Frustum, Scene, CmdContext);

						StaticMeshBatches AllStaticMeshBatches;
						UploadStaticMeshData(AllStaticMeshBatches, Frustum, Camera.ViewMatrix, Scene, CmdContext);

						BindGeometryPassBuffers(Scene);

						// TODO: Get rid of graph and just bind the relevant resources to the m_StaticMeshPass etc. instead
						const bool Succeeded = m_RenderGraph.Execute(m_GraphicsQueue, CmdContext, Camera, AllStaticMeshBatches, NumDirectionalLights, NumPointLights, NumSpotLights);

						if (!Succeeded)
						{
							DEBUG_PRINT("RenderGraph failed");
							return;
						}
					}
				}

				m_GraphicsQueue.ExecuteContext(CmdContext);
			}

			// TODO: Impl
			void Render(Scene::Scene::Camera& Camera, const std::vector<PrimitiveBatch*>& PrimitiveBatches, D3D12::RenderTargetView& RenderSurface, bool ClearRenderSurface, D3D12::DepthStencilView& DepthSurface, bool ClearDepthSurface)
			{
				auto CmdContextHandle = m_GraphicsCmdContextAllocator.GetContext();
				if (!CmdContextHandle.has_value())
				{
					throw std::runtime_error("No more command contexts");
				}

				D3D12::CommandContext& CmdContext = *CmdContextHandle->m_Context;

				if (Camera.IsActive)
				{
					for (const PrimitiveBatch* Batch : PrimitiveBatches)
					{
						this->RenderPrimitiveBatch(CmdContext, RenderSurface, *Batch, Camera);
					}
				}
			}
			
			void CompleteRender(ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture)
			{
				auto CmdContextHandle = m_GraphicsCmdContextAllocator.GetContext();
				if (!CmdContextHandle.has_value())
				{
					throw std::runtime_error("No more command contexts");
				}

				D3D12::CommandContext& CmdContext = *CmdContextHandle->m_Context;

				this->CopyRenderSurfaceToTexture(CmdContext, DstTexture, SrcTexture);
				m_GraphicsQueue.ExecuteContext(CmdContext);
			}
			//

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

			// TODO: Engine should own compiler and not the renderer
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

					auto CmdContext = m_GraphicsCmdContextAllocator.GetContext();
					if (!CmdContext.has_value())
					{
						throw std::runtime_error("No more command contexts");
					}
					ID3D12GraphicsCommandList* CmdList = CmdContext->m_Context->GetCommandList();

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

					CmdContext->m_Context->StopRecording();
					m_GraphicsQueue.ExecuteContext(*CmdContext->m_Context);
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

					auto CmdContext = m_GraphicsCmdContextAllocator.GetContext();
					if (!CmdContext.has_value())
					{
						throw std::runtime_error("No more command contexts");
					}
					ID3D12GraphicsCommandList* CmdList = CmdContext->m_Context->GetCommandList();
					D3D12::TransitionResources(CmdList, Bundle);

					D3D12_SUBRESOURCE_DATA SubresourceData{};
					SubresourceData.pData = Data.m_Data.data();
					SubresourceData.RowPitch = /*roundUp(*/Data.m_Dimensions.x * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
					SubresourceData.SlicePitch = SubresourceData.RowPitch * Data.m_Dimensions.y;

					UpdateSubresources(
						CmdList,
						NewEntry.Texture.GetResource(),
						StagingBuffer.GetResource(),
						0, 0, 1, &SubresourceData);

					Bundle.at(0).ResourcePtr = NewEntry.Texture.GetResource();
					Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
					Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					D3D12::TransitionResources(CmdList, Bundle);

					m_GraphicsQueue.ExecuteContext(*CmdContext->m_Context);

					NewEntry.SRV.Init(m_Device, m_ResourceHeap.GetDescriptor(), NewEntry.Texture, NewEntry.Texture.GetResource()->GetDesc().Format, 1, 0, 0, 0);

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

				auto CmdContext = m_GraphicsCmdContextAllocator.GetContext();
				if (!CmdContext.has_value())
				{
					throw std::runtime_error("No more command contexts");
				}
				ID3D12GraphicsCommandList* CmdList = CmdContext->m_Context->GetCommandList();
				m_MaterialBuffer.Write(CmdList, this->GetFrameAllocator(), m_AssetManager->GetRenderHandle(*MaterialAsset.get()).value()->MaterialAllocHandle, &RenderData);
				CmdContext->m_Context->StopRecording();
				m_GraphicsQueue.ExecuteContext(*CmdContext->m_Context);
			}
		};
	}
}