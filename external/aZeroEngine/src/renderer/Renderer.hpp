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
#include "ResourceManager.hpp"

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
			RenderAPI::ResourceRecycler& GetResourceRecycler() { return m_NewResourceRecycler; }
			RenderAPI::DescriptorHeap& GetResourceHeap() { return m_ResourceHeapNew; }
			RenderAPI::DescriptorHeap& GetSamplerHeap() { return m_SamplerHeapNew; }
			RenderAPI::DescriptorHeap& GetRenderTargetHeap() { return m_RTVHeapNew; }
			RenderAPI::DescriptorHeap& GetDepthTargetHeap() { return m_DSVHeapNew; }

			bool BeginFrame();
			void EndFrame();

			void Render(const Scene::SceneNew& scene, std::optional<Rendering::RenderTarget*> renderTarget, std::optional<Rendering::DepthTarget*> depthTarget);

			void CopyTextureToTexture(RenderAPI::Texture2D& dstTexture, RenderAPI::Texture2D& srcTexture);
			void CopyTextureToTexture(ID3D12Resource* dstTexture, ID3D12Resource* srcTexture);

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

			// Returns true if the frame context for the next frame has completed and is open for reuse
			bool AdvanceFrameIfReady();

			void InitPipeline();
			
			ID3D12DeviceX* m_diDevice;
			uint32_t m_BufferCount;
			uint32_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;


			// todo Figure out how this should be used to defer destruction of descriptors so that they wont be used until their no longer in use
			aZero::CallbackExecutor m_CallbackExecutor;

			IDxcCompilerX& m_Compiler;

			RenderAPI::CommandQueue m_DirectCommandQueue;
			RenderAPI::CommandQueue m_CopyCommandQueue;
			RenderAPI::CommandQueue m_ComputeCommandQueue;

			RenderAPI::ResourceRecycler m_NewResourceRecycler;
			RenderAPI::DescriptorHeap m_ResourceHeapNew;
			RenderAPI::DescriptorHeap m_SamplerHeapNew;

			SamplerManager m_SamplerManager;
			RenderAPI::DescriptorHeap m_RTVHeapNew;
			RenderAPI::DescriptorHeap m_DSVHeapNew;

			std::vector<FrameContext> m_FrameContexts;

			Rendering::ResourceManager m_ResourceManager;

			Pipeline::MeshShaderPass m_Pipeline;
			Pipeline::AmplificationShader m_AmpShader;
			Pipeline::MeshShader m_MeshShader;
			Pipeline::PixelShader m_PixelShader;
		};
	}
}