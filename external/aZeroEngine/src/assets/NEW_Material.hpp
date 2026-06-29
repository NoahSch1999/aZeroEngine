#pragma once
#include "NEW_RenderAssetBase.hpp"
#include "NEW_Texture.hpp"
#include "NEW_AssetManager.hpp"

namespace aZero
{
	namespace NEW_Asset
	{
		struct MaterialRenderRef
		{
			uint32_t MaterialIndex = std::numeric_limits<uint32_t>::max();

			bool IsValid() const
			{
				return MaterialIndex != std::numeric_limits<uint32_t>::max();
			}
		};

		struct MaterialData
		{
			std::string FilePath;
			std::string AlbedoTexture;
			std::string NormalMap;
		};

		class Material : public RenderAssetBase<MaterialRenderRef, MaterialData>
		{
			friend Rendering::Renderer;
		public:
			Material() = default;
			Material(MaterialData&& data)
				:RenderAssetBase(std::move(data)){}

			void SetAlbedo(NEW_Asset::Texture& albedo) { m_AlbedoRef = &albedo; }
			void SetNormalMap(NEW_Asset::Texture& normalMap) { m_NormalRef = &normalMap; }

			NEW_Asset::Texture* GetAlbedoPtr() const { return m_AlbedoRef; }
			NEW_Asset::Texture* GetNormalMapPtr() const { return m_NormalRef; }

		private:
			NEW_Asset::Texture* m_AlbedoRef = nullptr;
			NEW_Asset::Texture* m_NormalRef = nullptr;
		};
	}

	template<>
	inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<NEW_Asset::Material>(NEW_Asset::Material& material)
	{
		FrameContext& context = this->GetCurrentContext();

		uint32_t albedoIndex = material.m_AlbedoRef->GetRenderRef().DescriptorIndex;
		uint32_t normalIndex = material.m_NormalRef->GetRenderRef().DescriptorIndex;

		// todo Maybe call different overloads based on material properties?
		material.m_RenderRef.MaterialIndex = m_RenderAssetManager->UpdateRenderState(context.GetFrameStagingAllocator(), material.m_RenderRef.MaterialIndex, albedoIndex, normalIndex);
	}

	template<>
	inline void aZero::Rendering::Renderer::UnregisterAsset<NEW_Asset::Material>(NEW_Asset::Material& material)
	{
		m_RenderAssetManager->RemoveMaterialAsset(material.GetRenderRef().MaterialIndex);
	}

	template<>
	inline void aZero::Scene::Scene::OnAssetErased<NEW_Asset::Material>(NEW_Asset::Material& material)
	{
		m_World.defer_begin();
		m_World.query<Component::Mesh>().each(
			[material](flecs::entity entity, Component::Mesh& mesh) {
				for (uint32_t i = 0; i < mesh.m_NumSubmeshes; i++)
				{
					if (mesh.m_Submeshes[i].m_MaterialID == material.GetRenderRef().MaterialIndex)
					{
						entity.remove<Component::Mesh>();
					}
				}

			}
		);
		m_World.defer_end();
	}
}