#pragma once
#include "graphics_api/CommandQueue.hpp"
#include "renderer/render_graph/RenderGraph.hpp"
#include "ecs/aZeroECS.hpp"
#include "scene/SceneManager.hpp"
#include "graphics_api/DescriptorHeap.hpp"
#include "graphics_api/resource_type/FreelistBuffer.hpp"
#include "renderer/PrimitiveBatch.hpp"
#include "LinearAllocator.hpp"
#include "assets/Asset.hpp"
#include "pipeline/VertexShader.hpp"
#include "pipeline/PixelShader.hpp"

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

		class Renderer
		{
			friend Engine;
			friend RenderContext;
		private:
			void AllocateFreelistMesh(Asset::Mesh& mesh, ID3D12GraphicsCommandList* cmdList);
			void InitCommandRecording();
			void InitRenderPasses();
			void HotreloadRenderPasses();
			void InitFrameResources();
			void InitDescriptorHeaps();
			void InitSamplers();
			void UploadStagedAssets();

			Pipeline::PipelineManager* m_diPipelineManager = nullptr;
			ID3D12Device* m_diDevice = nullptr;

			D3D12::ResourceRecycler m_ResourceRecycler;
			D3D12::CommandQueue m_GraphicsQueue;
			D3D12::CommandContextAllocator m_CommandContextAllocator;

			Pipeline::VertexShader m_BasePassVS;
			Pipeline::PixelShader m_BasePassPS;

			D3D12::DescriptorHeap m_ResourceHeap;
			D3D12::DescriptorHeap m_SamplerHeap;
			D3D12::DescriptorHeap m_RTVHeap;
			D3D12::DescriptorHeap m_DSVHeap;

			uint32_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;
			uint32_t m_BufferCount;

			RenderGraphPass m_DefaultRenderPass;
			D3D12::Descriptor m_AnisotropicSampler;

			// TODO: Create shader resource views for them and access them bindlessly in the shaders
			std::vector<D3D12::GPUBuffer> m_StaticMeshFrameBuffers;
			std::vector<D3D12::GPUBuffer> m_PointLightFrameBuffers;
			std::vector<D3D12::GPUBuffer> m_SpotLightFrameBuffers;
			std::vector<D3D12::GPUBuffer> m_DirectionalLightFrameBuffers;

			D3D12::LinearFrameAllocator m_AssetStagingAllocator;

			// TODO: Make this way less copy-pasta and nicer
			// =============================================
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

			// Used to know where a material is located in the buffer
			struct MaterialHandle
			{
				uint32_t Index; // Index into the material buffer
			};
			// =============================================

		public:
			Renderer() = default;

			Renderer(ID3D12Device* device, uint32_t bufferCount, Pipeline::PipelineManager& diPipelineManager);

			// Marks the beginning of a frame.
			void BeginFrame();

			// Copies the rendered surface to the swapchain.
			// TODO: Impl up/downscaling to match the SrcTexture to the DstTexture (back buffer)
			void CopyTextureToTexture(ID3D12Resource* DstTexture, ID3D12Resource* SrcTexture);

			// Marks the end of a frame.
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
		};

	}
}