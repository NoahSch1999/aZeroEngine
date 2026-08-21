#pragma once
#include "scene/Scene.hpp"

namespace aZero::Asset
{
	// todo Make key work as more than string
	template<typename Key, typename ...AssetTypes>
	class AssetManagerT
	{
	public:
		AssetManagerT() = default;
		AssetManagerT(Rendering::Renderer& diRenderer, std::string_view projectRootDirectory)
			:m_diRenderer(&diRenderer), m_ProjectRootDirectory(projectRootDirectory) {

			Asset::TextureData::MipLevel mip;
			mip.Offset = 0;
			mip.RowPitch = 1 * sizeof(DWORD);
			mip.SlicePitch = mip.RowPitch * 1;

			Asset::TextureData fallbackTextureData;
			fallbackTextureData.Data = { 1,0,1,1 };
			fallbackTextureData.Format = RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM_SRGB;
			fallbackTextureData.Height = 1;
			fallbackTextureData.Width = 1;
			fallbackTextureData.MipPitchData.emplace_back(mip);
			Asset::Texture* fallbackTexture = this->Create<Asset::Texture>("Fallback", std::move(fallbackTextureData));

			Asset::TextureData fallbackNormalMapData;
			fallbackNormalMapData.Data = { 0,0,0,1 };
			fallbackNormalMapData.Format = RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM;
			fallbackNormalMapData.Height = 1;
			fallbackNormalMapData.Width = 1;
			fallbackNormalMapData.MipPitchData.emplace_back(mip);
			Asset::Texture* fallbackNormalMap = this->Create<Asset::Texture>("FallbackNormalMap", std::move(fallbackNormalMapData));

			Asset::TextureData fallbackMetallicRoughnessMapData;
			fallbackMetallicRoughnessMapData.Data = { 0,0,0,1 };
			fallbackMetallicRoughnessMapData.Format = RenderAPI::TEXTURE_FORMAT::RGBA8_UNORM;
			fallbackMetallicRoughnessMapData.Height = 1;
			fallbackMetallicRoughnessMapData.Width = 1;
			fallbackMetallicRoughnessMapData.MipPitchData.emplace_back(mip);
			Asset::Texture* fallbackMetallicRoughnessMap = this->Create<Asset::Texture>("FallbackMetallicRoughnessMap", std::move(fallbackMetallicRoughnessMapData));

			Asset::MaterialData fallbackMaterialData;
			fallbackMaterialData.Name = "Fallback";
			fallbackMaterialData.Info.AlbedoTexture = fallbackTexture;
			fallbackMaterialData.Info.NormalTexture = fallbackNormalMap;
			fallbackMaterialData.Info.MetallicRoughnessTexture = fallbackMetallicRoughnessMap;
			this->Create<Asset::Material>("Fallback", std::move(fallbackMaterialData));

			Asset::MeshData fallbackMeshData;
			fallbackMeshData.Name = "Fallback";
			fallbackMeshData.m_Submeshes.resize(1);
			fallbackMeshData.m_Submeshes[0].Bounds = DirectX::BoundingSphere({ 0.f,0.f,0.f }, 1.f);
			fallbackMeshData.m_Submeshes[0].MeshletCount = 0;
			fallbackMeshData.m_Submeshes[0].MeshletOffset = 0;
			fallbackMeshData.m_VertexData.Meshlets.resize(1);
			memset(&fallbackMeshData.m_VertexData.Meshlets[0], 0, sizeof(fallbackMeshData.m_VertexData.Meshlets[0]));
			fallbackMeshData.m_VertexData.Vertices.resize(1);
			memset(&fallbackMeshData.m_VertexData.Vertices[0], 0, sizeof(fallbackMeshData.m_VertexData.Vertices[0]));
			fallbackMeshData.m_VertexData.MeshletBounds.resize(1);
			memset(&fallbackMeshData.m_VertexData.MeshletBounds[0], 0, sizeof(fallbackMeshData.m_VertexData.MeshletBounds[0]));
			fallbackMeshData.m_VertexData.MeshletBounds[0].Radius = 1.f;

			this->Create<Asset::Mesh>("Fallback", std::move(fallbackMeshData));
		}

		template<typename AssetType>
		using AssetContainer = std::unordered_map<Key, std::unique_ptr<AssetType>>;

		template<typename AssetType>
		std::string GetAssetDirectory() {
			if (std::is_same<AssetType, Asset::Mesh>::value)
			{
				return m_ProjectRootDirectory + "meshes/";
			}
			else if (std::is_same<AssetType, Asset::Material>::value)
			{
				return m_ProjectRootDirectory + "materials/";
			}
			else if (std::is_same<AssetType, Asset::Texture>::value)
			{
				return m_ProjectRootDirectory + "textures/";
			}
			else
			{
				throw std::invalid_argument("Invalid asset type.");
			}
		}

		template<typename AssetType, typename ...CtorArgs>
		AssetType* Create(const Key& key, CtorArgs&&... args)
		{
			auto& container = std::get<AssetContainer<AssetType>>(m_AssetContainer);
			if (auto assetIter = container.find(key); assetIter != container.end())
			{
				return assetIter->second.get();
			}

			AssetType* asset = container.emplace(key, std::make_unique<AssetType>(std::forward<CtorArgs>(args)...)).first->second.get();

			m_diRenderer->RegisterOrUpdateAsset(*asset);

			return asset;
		}

		template<typename AssetType>
		bool Erase(const Key& key)
		{
			auto& container = std::get<AssetContainer<AssetType>>(m_AssetContainer);
			if (auto assetIter = container.find(key); assetIter != container.end())
			{
				this->EraseFromReferences(*assetIter->second.get());
				container.erase(assetIter);
				return true;
			}

			return false;
		}

		template<typename AssetType>
		AssetType* Get(const Key& key)
		{
			auto& container = std::get<AssetContainer<AssetType>>(m_AssetContainer);
			if (auto assetIter = container.find(key); assetIter != container.end())
			{
				return assetIter->second.get();
			}

			return nullptr;
		}

		template<typename AssetType>
		AssetType* GetOrDefault(const Key& key)
		{
			auto& container = std::get<AssetContainer<AssetType>>(m_AssetContainer);
			if (auto assetIter = container.find(key); assetIter != container.end())
			{
				return assetIter->second.get();
			}

			if constexpr (std::is_same<AssetType, Asset::Texture>::value)
			{
				return container.at("Fallback").get();
			}
			else if constexpr (std::is_same<AssetType, Asset::Material>::value)
			{
				return container.at("Fallback").get();
			}
			else if constexpr (std::is_same<AssetType, Asset::Mesh>::value)
			{
				return container.at("Fallback").get();
			}

			return nullptr;
		}

		template<typename AssetType>
		void Clear()
		{
			auto& container = std::get<AssetContainer<AssetType>>(m_AssetContainer);
			for (auto& [key, asset] : container)
			{
				this->EraseFromReferences(*asset.get());
			}
			container.clear();
		}

		void RegisterScene(Scene::Scene* stableScenePtr)
		{
			m_RegisteredScenes[stableScenePtr->GetSceneID()] = stableScenePtr;
		}

		void UnregisterScene(const Scene::Scene& scene)
		{
			if (auto sceneIter = m_RegisteredScenes.find(scene.GetSceneID()); sceneIter != m_RegisteredScenes.end())
			{
				m_RegisteredScenes.erase(sceneIter);
			}
		}

		template<typename AssetType>
		const AssetContainer<AssetType>& GetContainer() const { return std::get<AssetContainer<AssetType>>(m_AssetContainer); }

	private:
		template<typename AssetType>
		bool EraseFromReferences(AssetType& asset)
		{
			if constexpr (std::is_same<AssetType, Asset::Texture>::value) // Ugly, but needed for this case
			{
				auto& materials = std::get<AssetContainer<Asset::Material>>(m_AssetContainer);
				auto& textures = std::get<AssetContainer<Asset::Texture>>(m_AssetContainer);
				for (auto& [key, material] : materials)
				{
					bool hasUpdated = false;
					Asset::Texture* texture = static_cast<Asset::Texture*>(&asset);
					if (material->GetAlbedoPtr()->GetRenderRef().DescriptorIndex == texture->GetRenderRef().DescriptorIndex)
					{
						material->SetAlbedo(this->GetOrDefault<Asset::Texture>(textures[m_diRenderer->GetRenderAssetManager().GetDefaultTextureIndex()]));
						hasUpdated = true;
					}

					if (material->GetNormalMapPtr()->GetRenderRef().DescriptorIndex == texture->GetRenderRef().DescriptorIndex)
					{
						material->SetNormalMap(this->GetOrDefault<Asset::Texture>(textures[m_diRenderer->GetRenderAssetManager().GetDefaultTextureIndex()]));
						hasUpdated = true;
					}

					if (hasUpdated)
					{
						m_diRenderer->RegisterOrUpdateAsset(*material.get());
					}
				}
			}

			for (auto& [sceneID, scenePtr] : m_RegisteredScenes)
			{
				scenePtr->OnAssetErased(asset); // Force recache of static objects since their components need to be reuploaded
				scenePtr->MarkStaticMeshesDirty();
			}

			m_diRenderer->UnregisterAsset(asset);

			return true;
		}

		Rendering::Renderer* m_diRenderer = nullptr;
		std::tuple<AssetContainer<AssetTypes>...> m_AssetContainer;

		std::string m_ProjectRootDirectory;
		std::unordered_map<Scene::SceneID, Scene::Scene*> m_RegisteredScenes;
	};
}

