#pragma once
#include "ecs/aZeroECS.hpp"
#include "assets/Asset.hpp"
#include "pipeline/shader/VertexShader.hpp"
#include "pipeline/shader/PixelShader.hpp"
#include "misc/CallbackExecutor.hpp"
#include "graphics_api/resource/buffer/VertexBuffer.hpp"
#include "graphics_api/resource/buffer/IndexedBuffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"
#include "pipeline/pass/MeshShaderPass.hpp"
#include "pipeline/pass/VertexShaderPass.hpp"
#include "SamplerManager.hpp"
#include "FrameContext.hpp"
#include "scene/Scene.hpp"

namespace aZero
{
	class Engine;

	namespace Asset
	{
		class Mesh;
		class Material;
		class Texture;
	}

	namespace Rendering
	{
		class Renderer : public NonCopyable
		{
			friend class Engine;
		public:
			Renderer() = default;
			Renderer(ID3D12DeviceX* device, uint32_t bufferCount, IDxcCompilerX& compiler);
			Renderer(Renderer&&) noexcept = default;
			Renderer& operator=(Renderer&&) noexcept = default;

			size_t GetBufferingCount() const { return m_FrameContexts.size(); }
			RenderAPI::CommandQueue& GetGraphicsCommandQueue() { return m_DirectCommandQueue; }

			bool BeginFrame();

			void Render(const Scene::Scene& scene, std::optional<Rendering::RenderTarget*> renderTarget, std::optional<Rendering::DepthTarget*> depthTarget);

			void CopyTextureToTexture(RenderAPI::Texture2D& dstTexture, RenderAPI::Texture2D& srcTexture);

			void FlushGPU();
			uint64_t SignalGraphicsQueue() { return m_DirectCommandQueue.Signal(); }

			void UpdateRenderState(Asset::Mesh* mesh);
			void UpdateRenderState(Asset::Material* material);
			void UpdateRenderState(Asset::Texture* texture);
			void RemoveRenderState(Asset::Mesh* mesh);
			void RemoveRenderState(Asset::Material* material);
			void RemoveRenderState(Asset::Texture* texture);

		private:
			FrameContext& GetCurrentContext() { return m_FrameContexts.at(m_FrameIndex); }

			bool AdvanceFrameIfReady();

			void Init(ID3D12DeviceX* device, uint32_t numFramesInFlight);

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
			
			ID3D12DeviceX* m_diDevice;
			uint32_t m_BufferCount;
			uint32_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;

			RenderAPI::ResourceRecycler m_NewResourceRecycler;

			// todo Figure out how this should be used to defer destruction of descriptors so that they wont be used until their no longer in use
			aZero::CallbackExecutor m_CallbackExecutor;

			IDxcCompilerX& m_Compiler;

			RenderAPI::CommandQueue m_DirectCommandQueue;
			RenderAPI::CommandQueue m_CopyCommandQueue;
			RenderAPI::CommandQueue m_ComputeCommandQueue;

			SamplerManager m_SamplerManager;
			RenderAPI::DescriptorHeap m_ResourceHeapNew;
			RenderAPI::DescriptorHeap m_SamplerHeapNew;
			RenderAPI::DescriptorHeap m_RTVHeapNew;
			RenderAPI::DescriptorHeap m_DSVHeapNew;

			std::vector<FrameContext> m_FrameContexts;

			RenderAPI::IndexedBuffer<MaterialData> m_NewMaterialBuffer;
			std::unordered_map<Asset::RenderID, uint32_t> m_NewMaterialAllocMap;

			RenderAPI::IndexedBuffer<MeshEntry> m_MeshEntryBuffer;
			std::unordered_map<Asset::RenderID, uint32_t> m_NewMeshEntryAllocMap;

			std::unordered_map<Asset::RenderID, RenderAPI::VertexBuffer> m_GPUMeshes;
			std::unordered_map<Asset::RenderID, GPUTexture2D> m_GPUTextures;

			Pipeline::VertexShader m_NewBasePassVS;
			Pipeline::PixelShader m_NewBasePassPS;
		};

	}
}