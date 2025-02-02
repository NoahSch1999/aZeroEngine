#pragma once
#include "Renderer.hpp"

namespace aZero
{
	namespace Rendering
	{
		class RenderInterface
		{
		private:
			Renderer& m_Renderer;

		public:
			RenderInterface(Renderer& InRenderer)
				:m_Renderer(InRenderer)
			{

			}

			Asset::RenderAssetManager& GetAssetManager() { return *m_Renderer.m_AssetManager.get(); }

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
		};
	}
}