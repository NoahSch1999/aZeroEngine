#pragma once
#include "render_api/resource/texture/DepthStencilTarget.hpp"
#include "render_api/resource/texture/RenderTarget.hpp"
#include "SamplerManager.hpp"
#include "pipeline/RenderPass.hpp"
#include "GPU_Structs.hpp"
#include "misc/CallbackExecutor.hpp"
#include "RenderAssetManager.hpp"
#include "render_api/command_recording/CommandQueue.hpp"
#include "FrameContext.hpp"

namespace aZero
{
	namespace RenderAPI { class SwapChain; }
	namespace Pipeline { class RenderPass; class Shader; }
	namespace Scene { class Scene; }
}

namespace aZero::Rendering
{
	class FrameContext; class WireframeRenderer;

	class Renderer : public NonCopyable
	{
	public:
		struct RenderSettings
		{
			bool EnableDepthPrepass = false;
		};

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

		void Render(Scene::Scene& scene, Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget);

		void CopyRenderTargetToSwapChain(RenderAPI::SwapChain& swapChain, Rendering::RenderTarget& renderTarget);

		// This is specialized to define custom register-behavior on a per-asset level
		template<typename AssetType>
		void RegisterOrUpdateAsset(AssetType& asset);

		// This is specialized to define custom unregister-behavior on a per-asset level
		template<typename AssetType>
		void UnregisterAsset(AssetType& asset);

		void ClearRenderTarget(Rendering::RenderTarget& rtv);
		void ClearDepthStencilTarget(Rendering::DepthStencilTarget& dsv);

		Rendering::RenderTarget CreateRenderTarget(const Rendering::RenderTarget::Desc& desc);
		Rendering::DepthStencilTarget CreateDepthStencilTarget(const Rendering::DepthStencilTarget::Desc& desc);

		FrameContext& GetCurrentContext() { return m_FrameContexts.at(m_FrameIndex); }
		uint32_t GetFramesInFlight() const { return m_FrameContexts.size(); }

		// temp benchmark
		D3D12_VERTEX_BUFFER_VIEW temp_vbv;
		RenderAPI::Buffer temp_vBuffer;
		D3D12_VERTEX_BUFFER_VIEW temp_pbv;
		RenderAPI::Buffer temp_pBuffer;
		D3D12_INDEX_BUFFER_VIEW temp_ibv;
		RenderAPI::Buffer temp_iBuffer;
		void temp_LoadVB(FBX::FBX_Mesh& mesh);

		void CompilePipeline();

		RenderSettings GetRenderSettings() const { return m_RenderSettings; }

		void ToggleDepthPrepass(bool on) {
			if (on && !m_RenderSettings.EnableDepthPrepass) {
				m_RenderSettings.EnableDepthPrepass = on;
				this->CompilePipeline();
			}
			else if (!on && m_RenderSettings.EnableDepthPrepass) {
				m_RenderSettings.EnableDepthPrepass = on;
				this->CompilePipeline();
			}
		}

	private:

		// TODO: Remove and figure out a smooth way to replace it
		aZero::CallbackExecutor m_CallbackExecutor;
		RenderAPI::ResourceRecycler m_ResourceRecycler;

		// Returns true if the frame context for the next frame has completed and is open for reuse
		bool AdvanceFrameIfReady();

		uint32_t MAX_INSTANCES = 1000000;

		// ---------------------------------------------------------------------------------------------------------------------------------------------------

		void InitGPUDrivenRenderPipeline();
		void RecordGPUDrivenRenderPipeline(Rendering::RenderTarget& renderTarget, Rendering::DepthStencilTarget& depthStencilTarget, Scene::Scene& scene);

		// New version of GPU-driven
		Pipeline::RenderPass m_MeshCullPass;

		Pipeline::RenderPass m_MeshletDepthPass;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_MeshletDepthPassSignature;

		Pipeline::RenderPass m_MeshletDrawPass;

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

		SamplerManager m_SamplerManager;

		RenderAPI::DescriptorHeap m_ResourceHeap;
		RenderAPI::DescriptorHeap m_SamplerHeap;
		RenderAPI::DescriptorHeap m_RTVHeap;
		RenderAPI::DescriptorHeap m_DSVHeap;

		std::unique_ptr<Rendering::WireframeRenderer> m_WireframeRenderer;
		std::unique_ptr<Rendering::RenderAssetManager> m_RenderAssetManager;

		ID3D12DeviceX* m_diDevice;
		IDxcCompilerX& m_diCompiler;

		RenderSettings m_RenderSettings;
	};

	template<typename AssetType>
	inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset(AssetType&) { }

	template<typename AssetType>
	inline void aZero::Rendering::Renderer::UnregisterAsset(AssetType&) { }

	template<>
	inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<aZero::Asset::Mesh>(aZero::Asset::Mesh& mesh)
	{
		if (mesh.GetRenderRef().IsValid())
		{
			// todo Impl update of existing asset
			return;
		}
		FrameContext& context = this->GetCurrentContext();
		const auto [meshletOffset, vertexOffset] = m_RenderAssetManager->UpdateRenderState(context.GetFrameStagingAllocator(),
			mesh.GetCachedData().m_VertexData.Meshlets, mesh.GetCachedData().m_VertexData.Vertices, mesh.GetCachedData().m_VertexData.MeshletBounds);
		mesh.m_RenderRef.m_MeshletGlobalOffset = meshletOffset;
		mesh.m_RenderRef.m_VertexGlobalOffset = vertexOffset;
	}

	template<>
	inline void aZero::Rendering::Renderer::UnregisterAsset<aZero::Asset::Mesh>(aZero::Asset::Mesh& mesh)
	{
		m_RenderAssetManager->RemoveMeshAsset(mesh.GetRenderRef().m_MeshletGlobalOffset);
	}

	template<>
	inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<aZero::Asset::Material>(aZero::Asset::Material& material)
	{
		FrameContext& context = this->GetCurrentContext();

		if (!material.m_Info.AlbedoTexture->GetRenderRef().IsValid())
		{
			// todo Impl handling of textures non-valid textures
			throw std::invalid_argument("No bound valid albedo texture");
		}

		// todo Maybe call different overloads based on material properties?
		material.m_RenderRef.MaterialIndex = m_RenderAssetManager->UpdateRenderState(context.GetFrameStagingAllocator(), material.m_RenderRef.MaterialIndex, material.GetFormat_PBR_GPU());
	}

	template<>
	inline void aZero::Rendering::Renderer::UnregisterAsset<aZero::Asset::Material>(aZero::Asset::Material& material)
	{
		m_RenderAssetManager->RemoveMaterialAsset(material.GetRenderRef().MaterialIndex);
	}

	template<>
	inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<aZero::Asset::Texture>(Asset::Texture& texture)
	{
		if (texture.GetRenderRef().IsValid())
		{
			// todo Impl update of existing asset
			return;
		}
		FrameContext& context = this->GetCurrentContext();
		texture.m_RenderRef.DescriptorIndex = m_RenderAssetManager->UpdateRenderState(m_diDevice, context.GetCommandList(), m_ResourceRecycler, m_ResourceHeap, texture.GetCachedData());
		m_DirectCommandQueue.ExecuteCommandList(context.GetCommandList());
	}

	template<>
	inline void aZero::Rendering::Renderer::UnregisterAsset<aZero::Asset::Texture>(aZero::Asset::Texture& texture)
	{
		m_RenderAssetManager->RemoveTextureAsset(texture.GetRenderRef().DescriptorIndex);

		// todo Impl recycle that doesnt force flush of descriptors
		this->FlushRenderCommands();
	}
}