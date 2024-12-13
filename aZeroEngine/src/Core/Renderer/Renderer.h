#pragma once

// TODO: Minimize includes
#include "Core/Renderer/D3D12Wrap/Commands/CommandQueue.h"
#include "Core/Renderer/D3D12Wrap/Pipeline/RenderPass.h"
#include "Core/DataStructures/UniqueIndexList.h"
#include "Core/aZeroECSWrap/aZeroECS/aZeroECS.h"
#include "Core/Renderer/D3D12Wrap/Window/Window.h"
#include "RenderScene.h"
#include "Core/Renderer/D3D12Wrap/Descriptors/DescriptorHeap.h"
#include "Core/Renderer/D3D12Wrap/Window/SwapChain.h"
#include "Core/AssetTypes/Mesh.h"
#include "Core/AssetTypes/Texture.h"
#include "Core/AssetTypes/Material.h"
#include "Core/Renderer/D3D12Wrap/Resources/FreelistBuffer.h"

// TODO: Try to remove this being a dynamic string allocation etc...
// TODO: Add path for compiled shaders
#define SHADER_COMPILED_DIRECTORY std::string("?")
#define SHADER_COMPILED_PATH(ShaderName) (SHADER_COMPILED_DIRECTORY + std::string(ShaderName) + std::string(".?"))

#define SHADER_SRC_DIRECTORY std::string("src/Shaders/")
#define SHADER_SRC_PATH(ShaderName) (SHADER_SRC_DIRECTORY + std::string(ShaderName) + std::string(".hlsl"))

namespace aZero
{
	class Scene;
	class Engine;
	namespace Rendering
	{
		// TODO: Look over and move or replace (?)
		#define PRIMITIVE_DATA_WORLDMATRIX_OFFSET 0
		#define PRIMITIVE_DATA_MESHINDEX_OFFSET sizeof(DXM::Matrix)
		#define PRIMITIVE_DATA_MATERIALINDEX_OFFSET PRIMITIVE_DATA_MESHINDEX_OFFSET + sizeof(uint32_t)

		// TODO: Look over and move (?)
		struct PerDrawConstants
		{
			int PrimitiveIndex;
		};

		class RenderInterface;

		class Renderer
		{
			friend Engine;
			friend RenderInterface;

		private:
			ID3D12Device* m_Device;
			CComPtr<IDxcCompiler3> m_Compiler;

			D3D12::CommandQueue m_GraphicsQueue;
			uint64_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;
			uint64_t m_LastPresentSignal;

			D3D12::FreelistBuffer m_VertexBuffer;
			D3D12::FreelistBuffer m_IndexBuffer;
			D3D12::FreelistBuffer m_MeshEntryBuffer;
			D3D12::FreelistBuffer m_MaterialBuffer;

			D3D12::DescriptorHeap m_ResourceHeap;
			D3D12::DescriptorHeap m_SamplerHeap;
			D3D12::DescriptorHeap m_RTVHeap;
			D3D12::DescriptorHeap m_DSVHeap;

			// TODO: Remove/move once we have a more complex resource allocation strategy
			D3D12::ResourceRecycler m_ResourceRecycler;

			// TODO: Remove/change once we add it as an argument to the engine
			const uint64_t BUFFERING_COUNT = 3;

			// TODO: Remove/replace if we need to have a more complex per-frame copy strategy (ex. we need to somehow copy some stuff before continuing)
			std::vector<D3D12::LinearFrameAllocator> m_FrameAllocators;

			// TODO: Remove once CommandContext is sorted up
			D3D12::CommandContext m_GraphicsCommandContext;
			D3D12::CommandContext m_PackedGPULookupBufferUpdateContext;

			// TODO: Remove once we have a more smooth sampler and pipeline setup
			D3D12::Descriptor m_DefaultSamplerDescriptor;

			D3D12::GPUTexture m_FinalRenderSurface;
			D3D12::Descriptor m_FinalRenderSurfaceUAV;
			D3D12::Descriptor m_FinalRenderSurfaceRTV;

			D3D12::GPUTexture m_SceneDepthTexture;
			D3D12::Descriptor m_SceneDepthTextureDSV;

			//D3D12::RenderPass m_TempRenderPass;

			DXM::Vector2 m_RenderResolution;
			D3D12_CLEAR_VALUE m_RTVClearColor;
			D3D12_CLEAR_VALUE m_DSVClearColor;
			//

			// TODO: Remove once change the args of ::Render() to take in a scene, camera, and window and renders with it etc
			std::unordered_map<std::string, RenderScene> m_RenderScenes;
			Scene* m_CurrentScene = nullptr;

		private:

			void SetupRenderPipeline();

			void PrepareGPUBuffers();

			void RenderPrimitives();

			void CopyRenderSurfaceToBackBuffer(D3D12::SwapChain& SwapChain);

			void Present(D3D12::SwapChain& SwapChain)
			{
				SwapChain.Present();
				m_LastPresentSignal = m_GraphicsQueue.ForceSignal();
			}

		public:
			// TODO: Remove when index draw count is fully gpu driven
			std::unordered_map<uint32_t, std::shared_ptr<Asset::Mesh>> m_Entity_To_Mesh;
			D3D12::RenderPass Pass;

		protected:

			void BeginFrame();

			void Render(D3D12::SwapChain& SwapChain);

			void EndFrame();

			void ChangeRenderResolution(const DXM::Vector2& NewRenderResolution);

			void FlushImmediate() { m_GraphicsQueue.FlushImmediate(); }

		public:
			Renderer(ID3D12Device* Device, const DXM::Vector2& RenderResolution)
				:m_Device(Device)
			{	
				this->Init(Device, RenderResolution);
			}

			void Init(ID3D12Device* Device, const DXM::Vector2& RenderResolution);

			~Renderer()
			{
				m_GraphicsQueue.FlushCommands();
			}

			D3D12::LinearFrameAllocator& GetFrameAllocator()
			{
				return m_FrameAllocators[m_FrameIndex];
			}

			IDxcCompiler3& GetCompiler() { return *m_Compiler.p; }

			const DXM::Vector2& GetRenderResolution() const { return m_RenderResolution; }

			D3D12::CommandQueue& GetGraphicsQueue() { return m_GraphicsQueue; }

			RenderScene& CreateScene(const std::string& SceneName)
			{
				m_RenderScenes.emplace(SceneName, std::move(RenderScene(m_Device, 1000, 100, &m_ResourceRecycler)));
				return m_RenderScenes.at(SceneName);
			}

			void SetScene(Scene* InScene)
			{
				m_CurrentScene = InScene;
			}

			void MarkRenderStateDirty(Asset::Mesh& Mesh)
			{
				// TODO: Handle case of a zero-vertex mesh being marked dirty
				if (!Mesh.HasRenderState())
				{
					m_VertexBuffer.GetAllocation(
						Mesh.m_AssetGPUHandle.m_VertexBufferAllocHandle,
						Mesh.GetAssetData().Vertices.size() * sizeof(decltype(Mesh.GetAssetData().Vertices.at(0))));

					m_VertexBuffer.Write(this->GetFrameAllocator(), Mesh.m_AssetGPUHandle.m_VertexBufferAllocHandle, (void*)Mesh.GetAssetData().Vertices.data());

					m_IndexBuffer.GetAllocation(
						Mesh.m_AssetGPUHandle.m_IndexBufferAllocHandle,
						Mesh.GetAssetData().Indices.size() * sizeof(decltype(Mesh.GetAssetData().Indices.at(0))));

					m_IndexBuffer.Write(this->GetFrameAllocator(), Mesh.m_AssetGPUHandle.m_IndexBufferAllocHandle, (void*)Mesh.GetAssetData().Indices.data());

					m_MeshEntryBuffer.GetAllocation(
						Mesh.m_AssetGPUHandle.m_MeshEntryAllocHandle,
						sizeof(Asset::MeshGPUEntry)
					);

					Asset::MeshGPUEntry NewMeshEntry;
					NewMeshEntry.m_VertexStartOffset = Mesh.m_AssetGPUHandle.m_VertexBufferAllocHandle.GetStartOffset() / sizeof(Asset::VertexData);
					NewMeshEntry.m_IndexStartOffset = Mesh.m_AssetGPUHandle.m_IndexBufferAllocHandle.GetStartOffset() / sizeof(uint32_t);
					NewMeshEntry.m_NumIndices = Mesh.GetAssetData().Indices.size();
					m_MeshEntryBuffer.Write(this->GetFrameAllocator(), Mesh.m_AssetGPUHandle.m_MeshEntryAllocHandle, (void*)&NewMeshEntry);
				}
				else
				{
					// TODO: Impl case where a mesh asset that already has a render state will be modified
				}
			}

			int roundUp(int numToRound, int multiple)
			{
				if (multiple == 0)
					return numToRound;

				int remainder = numToRound % multiple;
				if (remainder == 0)
					return numToRound;

				return numToRound + multiple - remainder;
			}

			// TODO: Impl
			void MarkRenderStateDirty(Asset::Texture& Texture, DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				if (!Texture.HasRenderState())
				{
					Asset::TextureData& Data = Texture.m_AssetData;
					Asset::TextureGPUHandle& GPUHandle = Texture.m_AssetGPUHandle;

					GPUHandle.m_Texture.Init(
						m_Device,
						{ Data.m_Dimensions.x, Data.m_Dimensions.y, 1 },
						Format, D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
						1, // TODO: Mip should be in the m_AssetData cache
						D3D12_RESOURCE_STATE_COMMON,
						std::optional<D3D12_CLEAR_VALUE>{}, &m_ResourceRecycler
					);

					const UINT StagingBufferSize = GetRequiredIntermediateSize(GPUHandle.m_Texture.GetResource(), 0, 1);
					D3D12::GPUBuffer StagingBuffer;
					StagingBuffer.Init(m_Device, D3D12::GPUResource::CPUWRITE, StagingBufferSize, &m_ResourceRecycler);

					std::vector<D3D12::ResourceTransitionBundles> Bundle;
					Bundle.push_back({});
					Bundle.at(0).ResourcePtr = GPUHandle.m_Texture.GetResource();
					Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COMMON;
					Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
					D3D12::TransitionResources(m_GraphicsCommandContext.GetCommandList(), Bundle);

					D3D12_SUBRESOURCE_DATA SubresourceData{};
					SubresourceData.pData = Data.m_Data.data();
					SubresourceData.RowPitch = /*roundUp(*/Data.m_Dimensions.x * sizeof(DWORD)/*, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)*/;
					SubresourceData.SlicePitch = SubresourceData.RowPitch * Data.m_Dimensions.y;

					UpdateSubresources(
						m_GraphicsCommandContext.GetCommandList(),
						GPUHandle.m_Texture.GetResource(),
						StagingBuffer.GetResource(),
						0, 0, 1, &SubresourceData);

					Bundle.at(0).ResourcePtr = GPUHandle.m_Texture.GetResource();
					Bundle.at(0).StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
					Bundle.at(0).StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					D3D12::TransitionResources(m_GraphicsCommandContext.GetCommandList(), Bundle);

					m_GraphicsQueue.ExecuteContext(m_GraphicsCommandContext);

					GPUHandle.m_SRV.Init(m_Device, m_ResourceHeap, GPUHandle.m_Texture, GPUHandle.m_Texture.GetResource()->GetDesc().Format, 1, 0, 0, 0);
				}
				else
				{

				}
			}

			void MarkRenderStateDirty(Asset::Material& Material)
			{
				if (!Material.HasRenderState())
				{
					m_MaterialBuffer.GetAllocation(Material.m_AssetGPUHandle.m_MaterialAllocHandle, sizeof(Asset::MaterialRenderData));
				}
				
				Asset::MaterialRenderData RenderData = Material.GetRenderData();
				m_MaterialBuffer.Write(this->GetFrameAllocator(), Material.m_AssetGPUHandle.m_MaterialAllocHandle, &RenderData);
			}
		};

	}
}