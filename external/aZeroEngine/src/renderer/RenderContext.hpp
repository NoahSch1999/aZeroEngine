#pragma once
#include "Renderer.hpp"
#include "RenderSurface.hpp"
#include "assets/Mesh.hpp"

namespace aZero
{
	namespace Rendering
	{
		class RenderContextNew
		{
		public:
			RenderContextNew(RendererNew& renderer)
				:m_Renderer(renderer){}

			void BeginFrame()
			{
				m_Renderer.BeginFrame();
			}

			void CopySurfaceToResource(Rendering::RenderSurface& renderSurface, ID3D12Resource* dstTexture)
			{
				m_Renderer.CopyTextureToTexture(dstTexture, renderSurface.GetTexture().GetResource());
			}

			void EndFrame()
			{
				m_Renderer.EndFrame();
			}

			// TODO: This is OK for now, but needs to change once there's use multiple command queues etc.
			void FlushRenderingCommands()
			{
				m_Renderer.FlushGraphicsQueue();
			}

			void RecompileRenderPasses()
			{
				m_Renderer.InitRenderPasses();
			}

			// TODO: Implement for each type of asset, ex. mesh, material, texture
			void MarkRenderStateDirty(const AssetNew::Mesh& mesh)
			{

			}

		private:
			RendererNew& m_Renderer;
		};

		class RenderContext
		{
		private:
			Renderer& m_Renderer;

		public:
			RenderContext(Renderer& InRenderer)
				:m_Renderer(InRenderer)
			{

			}

			Asset::RenderAssetManager& GetAssetManager() { return *m_Renderer.m_AssetManager.get(); }

			void BeginRenderFrame()
			{
				m_Renderer.BeginFrame();
			}

			void Render(Scene::Scene& Scene, 
				const Rendering::RenderSurface& RenderSurfaces,
				bool ClearRenderSurface, 
				const Rendering::RenderSurface& DepthSurface,
				bool ClearDepthSurface
			)
			{
				m_Renderer.Render(
					Scene, 
					RenderSurfaces.GetView<D3D12::RenderTargetView>(),
					ClearRenderSurface, 
					DepthSurface.GetView<D3D12::DepthStencilView>(),
					ClearDepthSurface
				);
			}

			void Render(Scene::Scene::Camera& Camera, 
				const std::vector<Rendering::PrimitiveBatch*>& PrimitiveBatches, 
				const Rendering::RenderSurface& RenderSurface,
				bool ClearRenderSurface,
				const Rendering::RenderSurface& DepthSurface,
				bool ClearDepthSurface
			)
			{
				//m_Renderer.Render(Camera, PrimitiveBatches, RenderSurface, ClearRenderSurface, DepthSurface, ClearDepthSurface);
			}

			void CompleteRender(Rendering::RenderSurface& RenderSurface, std::shared_ptr<Window::RenderWindow> Window)
			{
				m_Renderer.CompleteRender(Window->GetBackCurrentBuffer(), RenderSurface.m_Texture.get()->GetResource());
			}

			void EndRenderFrame()
			{
				m_Renderer.EndFrame();
			}

			void FlushRenderingCommands()
			{
				m_Renderer.FlushImmediate();
			}

			D3D12::ResourceRecycler& GetResourceRecycler() { return m_Renderer.m_ResourceRecycler; }

			std::optional<D3D12::CommandContextAllocator::CommandContextHandle> GetCommandContext() 
			{ 
				return m_Renderer.m_GraphicsCmdContextAllocator.GetContext(); 
			}

			D3D12::CommandQueue& GetGraphicsQueue() { return m_Renderer.m_GraphicsQueue; }

			uint32_t GetFramesInFlight() const { return m_Renderer.m_BufferCount; }

			D3D12::DescriptorHeap& GetRtvHeap() { return m_Renderer.m_RTVHeap; }
			D3D12::DescriptorHeap& GetDsvHeap() { return m_Renderer.m_DSVHeap; }

			void MarkRenderStateDirty(const std::shared_ptr<Asset::Mesh>& MeshAsset)
			{
				m_Renderer.MarkRenderStateDirty(MeshAsset);
			}

			void MarkRenderStateDirty(const std::shared_ptr<Asset::Texture>& TextureAsset)
			{
				m_Renderer.MarkRenderStateDirty(TextureAsset);
			}

			void MarkRenderStateDirty(const std::shared_ptr<Asset::Material>& MaterialAsset)
			{
				m_Renderer.MarkRenderStateDirty(MaterialAsset);
			}

			/* TODO per asset: 
					-Hitta "plats" i gpu resource
					-Ladda upp datan
					-Hämta index och sätt assettens renderID till det
					OBS: casta error/logga om något går fel, inte säker på hur det ska hanteras. Sätt iaf inte renderID till ett valid index
			*/
			void MarkRenderStateDirt(AssetNew::Mesh& mesh)
			{

			}

			void MarkRenderStateDirt(AssetNew::Material& material)
			{

			}

			void MarkRenderStateDirt(AssetNew::Texture& texture)
			{

			}
		};
	}
}