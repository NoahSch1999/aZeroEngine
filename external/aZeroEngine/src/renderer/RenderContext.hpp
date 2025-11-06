#pragma once
#include "Renderer.hpp"
#include "RenderSurface.hpp"
#include "assets/AssetManager.hpp"
#include "scene/SceneManager.hpp"

namespace aZero
{
	namespace Rendering
	{
		class RenderContext
		{
		public:
			RenderContext(Renderer& renderer)
				:m_Renderer(renderer){}

			void BeginFrame()
			{
				m_Renderer.BeginFrame();
			}

			void Render(Scene::Scene& Scene,
				Rendering::RenderSurface& RenderSurfaces,
				bool ClearRenderSurface,
				Rendering::RenderSurface& DepthSurface,
				bool ClearDepthSurface
			)
			{
				m_Renderer.Render(
					Scene,
					RenderSurfaces,
					ClearRenderSurface,
					DepthSurface,
					ClearDepthSurface
				);
			}

			void CompleteRender(Rendering::RenderSurface& RenderSurface, std::shared_ptr<Window::RenderWindow> Window)
			{
				m_Renderer.CopyTextureToTexture(Window->GetBackCurrentBuffer(), RenderSurface.m_Texture.get()->GetResource());
			}

			void EndFrame()
			{
				m_Renderer.EndFrame();
			}

			// TODO: This is OK for now, but needs to change once there's multiple command queues.
			void FlushRenderingCommands()
			{
				m_Renderer.FlushGraphicsQueue();
			}

			void HotreloadRenderPipeline()
			{
				m_Renderer.HotreloadRenderPasses();
			}

			/**
			* @brief Updates GPU render state for a given mesh asset.
			*
			* @param mesh Reference to an `Asset::AssetHandle<Asset::Mesh>` representing the mesh to update.
			*
			* @details
			* This function uploads or refreshes the relevant asset data to VRAM.
			*
			* - If the mesh has not yet been assigned a `RenderID`, it's assigned a new ID from the Renderer.
			* - If the mesh already exists on the GPU, it is reallocated to VRAM with the new data in a safe way.
			* - The function exits early if the AssetHandle is invalid.
			*/
			void UpdateRenderState(Asset::AssetHandle<Asset::Mesh>& mesh)
			{
				this->m_Renderer.UpdateRenderState(mesh);
			}

			/**
			* @brief Updates GPU render state for a given material asset.
			*
			* @param material Reference to an `Asset::AssetHandle<Asset::Material>` representing the material to update.
			*
			* @details
			* This function uploads or refreshes the relevant asset data to VRAM.
			* 
			* - If the material has not been assigned a `RenderID`, it's assigned a new ID from the Renderer.
			* - The texture asset index (that is used to access the texture through ResourceDescriptorHeap in the shaders) will be uploaded as `-1` to VRAM for the texture handles that are either invalid or haven't been assigned a RenderID.
			* - The function exits early if the AssetHandle is invalid.
			*
			* @warning
			* Make sure the associated textures have been uploaded via
			* `Renderer::UpdateRenderState(Asset::AssetHandle<Asset::Texture>&)` before
			* calling this function to avoid invalid texture references.
			*/
			void UpdateRenderState(Asset::AssetHandle<Asset::Material>& material)
			{
				this->m_Renderer.UpdateRenderState(material);
			}

			/**
			* @brief Updates GPU render state for a given texture asset.
			*
			* @param texture Reference to an `Asset::AssetHandle<Asset::Texture>` representing the texture to update.
			*
			* @details
			* This function uploads CPU-side texture data (`TexelData`) into a GPU texture resource.
			* 
			* - If the texture has not been assigned a `RenderID`, it's assigned a new ID from the Renderer.
			* - The function exits early if the AssetHandle is invalid or if the texel data doesn't match the texture's dimensions.
			*
			* @warning
			* Ensure that no GPU commands are using the previous texture when the same descriptor index
			* is reallocated, as this may result in undefined behavior or rendering artifacts.
			*/
			void UpdateRenderState(Asset::AssetHandle<Asset::Texture>& texture)
			{
				this->m_Renderer.UpdateRenderState(texture);
			}

			Pipeline::ScenePass*& GetScenePass() { return m_Renderer.scenePass; }

		private:
			Renderer& m_Renderer;
		};
	}
}