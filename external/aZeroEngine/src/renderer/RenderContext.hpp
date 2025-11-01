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

			void UpdateRenderState(Asset::Mesh& mesh)
			{
				this->m_Renderer.UpdateRenderState(mesh);
			}

			void UpdateRenderState(Asset::Material& material)
			{
				this->m_Renderer.UpdateRenderState(material);
			}

			void UpdateRenderState(Asset::Texture& texture)
			{
				this->m_Renderer.UpdateRenderState(texture);
			}

		private:
			Renderer& m_Renderer;
		};
	}
}