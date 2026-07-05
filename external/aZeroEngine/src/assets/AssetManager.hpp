#pragma once
#include "scene/Scene.hpp"

namespace aZero::Asset
{
	template<typename Key, typename ...AssetTypes>
	class AssetManagerT
	{
	public:
		AssetManagerT() = default;
		AssetManagerT(Rendering::Renderer& diRenderer, std::string_view projectRootDirectory)
			:m_diRenderer(&diRenderer), m_ProjectRootDirectory(projectRootDirectory) { }

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
				throw;
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

			if constexpr (std::is_same<AssetType, Asset::Material>::value) // Ugly, but needed for this case
			{
				Asset::Material* material = static_cast<Asset::Material*>(asset);

				const std::string& albedoName = material->GetCachedData().AlbedoTexture;
				if (!albedoName.empty()) {
					Asset::Texture* albedo = this->Get<Asset::Texture>(albedoName);
					if (!albedo) {
						Asset::TextureData texData;
						texData.Load(this->GetAssetDirectory<Asset::Texture>() + albedoName, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
						this->Create<Asset::Texture>(albedoName, std::move(texData));
						albedo = this->Get<Asset::Texture>(albedoName);
					}
					material->SetAlbedo(*albedo);
				}

				const std::string& normalMapName = material->GetCachedData().NormalMap;
				if (!normalMapName.empty()) {
					Asset::Texture* normalMap = this->Get<Asset::Texture>(normalMapName);
					if (!normalMap) {
						Asset::TextureData texData;
						texData.Load(this->GetAssetDirectory<Asset::Texture>() + normalMapName, DXGI_FORMAT_R8G8B8A8_UNORM);
						this->Create<Asset::Texture>(normalMapName, std::move(texData));
						normalMap = this->Get<Asset::Texture>(normalMapName);
					}
					material->SetNormalMap(*normalMap);
				}
			}

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
						material->SetAlbedo(this->Get<Asset::Texture>(textures[m_diRenderer->GetRenderAssetManager().GetDefaultTextureIndex()]));
						hasUpdated = true;
					}

					if (material->GetNormalMapPtr()->GetRenderRef().DescriptorIndex == texture->GetRenderRef().DescriptorIndex)
					{
						material->SetNormalMap(this->Get<Asset::Texture>(textures[m_diRenderer->GetRenderAssetManager().GetDefaultTextureIndex()]));
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