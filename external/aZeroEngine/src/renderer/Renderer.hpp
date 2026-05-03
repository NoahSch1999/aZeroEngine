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
#include "pipeline/pass/ComputeShaderPass.hpp"
#include "pipeline/pass/VertexShaderPass.hpp"
#include "SamplerManager.hpp"
#include "FrameContext.hpp"
#include "scene/Scene.hpp"
#include "ResourceManager.hpp"
#include "graphics_api/resource/texture/DepthStencilTarget.hpp"
#include "graphics_api/resource/texture/RenderTarget.hpp"
#include "graphics_api/SwapChain.hpp"
#include "renderer/RenderPass.hpp"

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
		class WireframeRenderer;

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
			RenderAPI::DescriptorHeap& GetResourceHeap() { return m_ResourceHeap; }
			RenderAPI::DescriptorHeap& GetSamplerHeap() { return m_SamplerHeap; }
			Rendering::WireframeRenderer& GetWireframeRenderer();
			uint32_t GetFrameIndex() const { return m_FrameIndex; }

			void FlushFrameAllocations();
			void FlushGPU();

			bool TryBeginFrame();
			void EndFrame();

			void Render(const Scene::Scene& scene);
			void RenderWireframes(const ECS::CameraComponent& camera, Rendering::RenderTarget& rtv, Rendering::DepthStencilTarget& dsv);
			void CopyRenderTargetToSwapChain(RenderAPI::SwapChain& swapChain, Rendering::RenderTarget& renderTarget);

			void UpdateRenderState(Asset::Mesh& mesh);
			void UpdateRenderState(Asset::Material& material);
			void UpdateRenderState(Asset::Texture& texture);

			// TODO: Impl
			void RemoveRenderState(Asset::Mesh& mesh);
			void RemoveRenderState(Asset::Material& material);
			void RemoveRenderState(Asset::Texture& texture);

			Rendering::RenderTarget CreateRenderTarget(const Rendering::RenderTarget::Desc& desc);
			Rendering::DepthStencilTarget CreateDepthStencilTarget(const Rendering::DepthStencilTarget::Desc& desc);

			FrameContext& GetCurrentContext() { return m_FrameContexts.at(m_FrameIndex); }

			/*void ExecuteRenderPasses();
			std::vector<Rendering::PassBase*> m_RenderPasses;*/
		private:

			// Returns true if the frame context for the next frame has completed and is open for reuse
			bool AdvanceFrameIfReady();

			void InitPipeline();
			void ClearRenderSurfaces(const Scene::RenderData::Camera& camera);

			/*
			---------------------------------------------------------------------------------------------------------------------------------------------
			Instance -> Meshlet -> Mesh shader pipeline
			---------------------------------------------------------------------------------------------------------------------------------------------
			*/
			struct BindingConstants
			{
				uint32_t InstanceBuffer;
				uint32_t MeshBuffer;
				uint32_t CameraBuffer;
				uint32_t CameraID;
				uint32_t IndirectArgumentMeshletCullingBuffer;
				uint32_t MeshletInstanceBuffer;
			};
			void InitMeshObjectCullPipeline();
			void InitMeshletCullPipeline();
			void InitMeshletDrawPipeline();
			void RecordMeshObjectCullingPass(const BindingConstants& bindings, uint32_t numStaticMeshes);
			void RecordMeshLetCullingPass(const BindingConstants& bindings);
			void RecordMeshDrawingPass(const BindingConstants& bindings, const Scene::RenderData::Camera& camera, uint32_t pointLightBufferIndex, uint32_t spotLightBufferIndex, uint32_t directionalLightBufferIndex);

			uint32_t MAX_INSTANCES = 4000; // TODO: Make configurable
			uint32_t MAX_MESHLETS = 10000; // TODO: Make configurable
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_MeshletDrawSignature;
			RenderAPI::Buffer m_MeshletDrawArgumentBuffer;
			RenderAPI::UnorderedAccessView m_MeshletDrawArgumentUAV;
			RenderAPI::Buffer m_MeshletInstanceBuffer;
			RenderAPI::UnorderedAccessView m_MeshletInstanceUAV;
			Pipeline::MeshShaderPass m_MeshletDrawPass;
			Pipeline::MeshShader m_MeshletDrawMS;
			Pipeline::PixelShader m_MeshletDrawPS;

			Pipeline::ComputeShaderPass m_MeshletCullingPass;
			Pipeline::ComputeShader m_MeshletCullingCS;

			Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_MeshObjectCullSignature;
			Pipeline::ComputeShaderPass m_MeshObjectCullingPass;
			Pipeline::ComputeShader m_MeshObjectCullingCS;
			RenderAPI::Buffer m_MeshObjectCullingBuffer;
			RenderAPI::UnorderedAccessView m_MeshObjectCullingUAV;
			RenderAPI::Buffer m_PassedMeshCountBuffer;
			RenderAPI::UnorderedAccessView m_PassedMeshCountUAV;

			/*
			---------------------------------------------------------------------------------------------------------------------------------------------
			*/


			
			ID3D12DeviceX* m_diDevice;
			uint32_t m_BufferCount;
			uint32_t m_FrameIndex = 0;
			uint64_t m_FrameCount = 0;

			// TODO: Figure out how this should be used to defer destruction of descriptors so that they wont be used until their no longer in use
			aZero::CallbackExecutor m_CallbackExecutor;

			IDxcCompilerX& m_Compiler;

			RenderAPI::CommandQueue m_DirectCommandQueue;
			RenderAPI::CommandQueue m_CopyCommandQueue;
			RenderAPI::CommandQueue m_ComputeCommandQueue;

			RenderAPI::ResourceRecycler m_ResourceRecycler;
			RenderAPI::DescriptorHeap m_ResourceHeap;
			RenderAPI::DescriptorHeap m_SamplerHeap;

			SamplerManager m_SamplerManager;
			RenderAPI::DescriptorHeap m_RTVHeap;
			RenderAPI::DescriptorHeap m_DSVHeap;

			std::vector<FrameContext> m_FrameContexts;

			Rendering::ResourceManager m_ResourceManager;

			std::unique_ptr<Rendering::WireframeRenderer> m_WireframeRenderer;
		};
	}
}