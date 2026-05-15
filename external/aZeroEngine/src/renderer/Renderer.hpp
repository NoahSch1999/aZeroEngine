#pragma once
#include "graphics_api/resource/texture/DepthStencilTarget.hpp"
#include "graphics_api/resource/texture/RenderTarget.hpp"
#include "SamplerManager.hpp"
#include "ResourceManager.hpp"

// TODO: Remove once replaced
#include "pipeline/shader/PixelShader.hpp"
#include "misc/CallbackExecutor.hpp"
#include "pipeline/pass/MeshShaderPass.hpp"
#include "pipeline/pass/ComputeShaderPass.hpp"

namespace aZero
{
	namespace Asset { class Mesh; class Material; class Texture; }
	namespace RenderAPI { class MeshBuffer; class SwapChain; }
	namespace NEW_Pipeline { class RenderPass; class Shader; }
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

		// Returns true if the frame context for the next frame has completed and is open for reuse
		bool AdvanceFrameIfReady();

		/*
		---------------------------------------------------------------------------------------------------------------------------------------------
		GPU Types
		---------------------------------------------------------------------------------------------------------------------------------------------
		*/
		

		//

		uint32_t MAX_INSTANCES = 1000000; // TODO: Make configurable
		uint32_t MAX_MESHLETS = 1000000; // TODO: Make configurable

		// ---------------------------------------------------------------------------------------------------------------------------------------------------

		void InitGPUDrivenRenderPipeline_NEW();
		void RecordGPUDrivenRenderPipeline_NEW(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, const Rendering::GPUProxy::Camera& camera, uint32_t numStaticMeshes);

		// New version of GPU-driven
		NEW_Pipeline::RenderPass m_MeshCullPass_NEW;
		NEW_Pipeline::Shader m_MeshCullCS_NEW;

		NEW_Pipeline::RenderPass m_MeshletDrawPass_NEW;
		NEW_Pipeline::Shader m_MeshletDrawAS_NEW;
		NEW_Pipeline::Shader m_MeshletDrawMS_NEW;
		NEW_Pipeline::Shader m_MeshletDrawPS_NEW;

		// ---------------------------------------------------------------------------------------------------------------------------------------------------

		void InitGPUDrivenRenderPipeline();
		void RecordGPUDrivenRenderPipeline(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, const Rendering::GPUProxy::Camera& camera, uint32_t numStaticMeshes);

		// Geometry render pipeline
		RenderAPI::Buffer m_MeshInstanceBuffer;

		Pipeline::ComputeShaderPass m_MeshCullPass;
		Pipeline::ComputeShader m_MeshCullCS;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_MeshletCullSignature;

		Pipeline::MeshShaderPass m_MeshletDrawPass;
		Pipeline::AmplificationShader m_MeshletDrawAS;
		Pipeline::MeshShader m_MeshletDrawMS;
		Pipeline::PixelShader m_MeshletDrawPS;

		struct GPUDrivenRenderConstants
		{
			DXM::Matrix CameraView; // Camera view matrix
			Component::Camera::BoundingFrustum CameraFrustum; // Camera frustum
			uint32_t MeshInstancesCount; // Num meshinstances to perform frustum-culling with
			uint32_t pad2[3];
		};

		struct MeshCull_Count {
			uint32_t Count;
		};
		RenderAPI::Buffer m_MeshCull_Count_B; // - MeshCull_Count - Used to interlock_add and get the IA argument index for the MeshletCull_IA_B buffer - Read/Written to in the MeshCull pass via UAV

		struct MeshletDrawConstantsData
		{
			DXM::Matrix WorldTransform;
			uint32_t MeshBuffer_Bindless;
			uint32_t MeshletCount;
			uint32_t pad[2];
		};

		struct MaterialConstantsData
		{
			uint32_t MaterialIndex;
		};

		struct MeshletCull_IA {
			MeshletDrawConstantsData MeshInstance; // Index into the framecontext's meshinstance buffer
			MaterialConstantsData MaterialConstants;
			uint32_t GroupsX; // Doesn't need to be reset since it's overwritten fully each time it's used
			uint32_t GroupsY; // Always 1
			uint32_t GroupsZ; // Always 1
		};
		RenderAPI::Buffer m_MeshletCull_IA_B; // - MeshletCull_IA - Contains IA arguments for the MeshletCull pass - Written to in the MeshCull pass via UAV and consumed by executeindirect by the MeshletCull pass

		RenderAPI::Buffer m_CameraBuffer;
		/*
		---------------------------------------------------------------------------------------------------------------------------------------------
		*/
		
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

		// TODO: Figure out how this should be used to defer destruction of descriptors so that they wont be used until their no longer in use
		aZero::CallbackExecutor m_CallbackExecutor;
		RenderAPI::ResourceRecycler m_ResourceRecycler;

		ID3D12DeviceX* m_diDevice;
		IDxcCompilerX& m_diCompiler;
	};
}