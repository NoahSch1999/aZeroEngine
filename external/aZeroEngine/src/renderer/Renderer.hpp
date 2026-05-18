#pragma once
#include "graphics_api/resource/texture/DepthStencilTarget.hpp"
#include "graphics_api/resource/texture/RenderTarget.hpp"
#include "SamplerManager.hpp"
#include "ResourceManager.hpp"
#include "pipeline/RenderPass.hpp"
#include "GPU_Driven_Pipeline_Structs.hpp"
#include "misc/CallbackExecutor.hpp"

namespace aZero
{
	namespace Asset { class Mesh; class Material; class Texture; }
	namespace RenderAPI { class MeshBuffer; class SwapChain; }
	namespace Pipeline { class RenderPass; class Shader; }
	namespace Scene { class Scene; }
}

namespace aZero::Rendering
{
	class FrameContext; class WireframeRenderer;

	class Renderer : public NonCopyable
	{
	public:
		Renderer() = default;
		Renderer(ID3D12DeviceX* device, uint32_t bufferCount, IDxcCompilerX& compiler);
		Renderer(Renderer&&) noexcept = default;
		Renderer& operator=(Renderer&&) noexcept = default;

		size_t GetBufferingCount() const { return m_FrameContexts.size(); }
		RenderAPI::CommandQueue& GetGraphicsCommandQueue() { return m_DirectCommandQueue; }
		RenderAPI::DescriptorHeap& GetResourceHeap() { return m_ResourceHeap; }
		RenderAPI::DescriptorHeap& GetSamplerHeap() { return m_SamplerHeap; }
		Rendering::WireframeRenderer& GetWireframeRenderer();
		uint32_t GetFrameIndex() const { return m_FrameIndex; }

		void FlushFrameAllocations();
		void FlushRenderCommands();

		bool TryBeginFrame();
		void EndFrame();

		void Render(const Scene::Scene& scene, Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget);

		void CopyRenderTargetToSwapChain(RenderAPI::SwapChain& swapChain, Rendering::RenderTarget& renderTarget);

		void UpdateRenderState(Asset::Mesh& mesh);
		void UpdateRenderState(Asset::Material& material);
		void UpdateRenderState(Asset::Texture& texture);

		// TODO: Impl
		void RemoveRenderState(Asset::Mesh& mesh);
		void RemoveRenderState(Asset::Material& material);
		void RemoveRenderState(Asset::Texture& texture);

		void ClearRenderTarget(Rendering::RenderTarget& rtv);
		void ClearDepthStencilTarget(Rendering::DepthStencilTarget& dsv);

		Rendering::RenderTarget CreateRenderTarget(const Rendering::RenderTarget::Desc& desc);
		Rendering::DepthStencilTarget CreateDepthStencilTarget(const Rendering::DepthStencilTarget::Desc& desc);

		FrameContext& GetCurrentContext() { return m_FrameContexts.at(m_FrameIndex); }
		uint32_t GetFramesInFlight() const { return m_FrameContexts.size(); }

	private:

		// TODO: Figure out how this should be used to defer destruction of descriptors so that they wont be used until their no longer in use
		aZero::CallbackExecutor m_CallbackExecutor;
		RenderAPI::ResourceRecycler m_ResourceRecycler;

		// Returns true if the frame context for the next frame has completed and is open for reuse
		bool AdvanceFrameIfReady();

		/*
		---------------------------------------------------------------------------------------------------------------------------------------------
		GPU Types
		---------------------------------------------------------------------------------------------------------------------------------------------
		*/

		//

		uint32_t MAX_INSTANCES = 1000000; // TODO: Make configurable

		// ---------------------------------------------------------------------------------------------------------------------------------------------------

		void InitGPUDrivenRenderPipeline_NEW();
		void RecordGPUDrivenRenderPipeline_NEW(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, const Rendering::GPUProxy::Camera& camera, uint32_t numStaticMeshes);

		// New version of GPU-driven
		Pipeline::RenderPass m_MeshCullPass_NEW;
		Pipeline::Shader m_MeshCullCS_NEW;

		Pipeline::RenderPass m_MeshletDrawPass_NEW;
		Pipeline::Shader m_MeshletDrawAS_NEW;
		Pipeline::Shader m_MeshletDrawMS_NEW;
		Pipeline::Shader m_MeshletDrawPS_NEW;

		RenderAPI::Buffer m_IndirectArguments;
		RenderAPI::Buffer m_IndirectArgumentCounter;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_MeshletDrawSignature;

		// ---------------------------------------------------------------------------------------------------------------------------------------------------
		
		uint32_t m_FrameIndex = 0;
		uint64_t m_FrameCount = 0;

		std::vector<FrameContext> m_FrameContexts;

		RenderAPI::CommandQueue m_DirectCommandQueue;
		RenderAPI::CommandQueue m_CopyCommandQueue;
		RenderAPI::CommandQueue m_ComputeCommandQueue;

		Rendering::ResourceManager m_ResourceManager;
		SamplerManager m_SamplerManager;

		RenderAPI::DescriptorHeap m_ResourceHeap;
		RenderAPI::DescriptorHeap m_SamplerHeap;
		RenderAPI::DescriptorHeap m_RTVHeap;
		RenderAPI::DescriptorHeap m_DSVHeap;

		std::unique_ptr<Rendering::WireframeRenderer> m_WireframeRenderer;

		ID3D12DeviceX* m_diDevice;
		IDxcCompilerX& m_diCompiler;
	};
}