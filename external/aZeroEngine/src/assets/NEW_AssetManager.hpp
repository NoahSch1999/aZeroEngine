#pragma once
#include "render_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"
#include "misc/HelperFunctions.hpp"
#include "scene/Scene.hpp"
#include "MeshPrimitives.hpp"

#include "assets/NEW_Mesh.hpp"
#include "assets/NEW_Material.hpp"
#include "assets/NEW_Texture.hpp"

namespace aZero
{
	namespace NEW_Asset
	{
		template<typename Key, typename ...AssetTypes>
		class AssetManager
		{
		public:
			AssetManager() = default;
			AssetManager(Rendering::Renderer& diRenderer)
				:m_diRenderer(&diRenderer)
			{

			}

			template<typename AssetType>
			using AssetContainer = std::unordered_map<Key, std::unique_ptr<AssetType>>;

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
				if constexpr (std::is_same<AssetType, NEW_Asset::Texture>::value) // Ugly, but needed for this case
				{
					auto& materials = std::get<AssetContainer<NEW_Asset::Material>>(m_AssetContainer);
					auto& textures = std::get<AssetContainer<NEW_Asset::Texture>>(m_AssetContainer);
					for (auto& [key, material] : materials)
					{
						bool hasUpdated = false;
						NEW_Asset::Texture* texture = static_cast<NEW_Asset::Texture*>(&asset);
						if (material->GetAlbedoPtr()->GetRenderRef().DescriptorIndex == texture->GetRenderRef().DescriptorIndex)
						{
							material->SetAlbedo(this->Get<NEW_Asset::Texture>(textures[m_diRenderer->GetRenderAssetManager().GetDefaultTextureIndex()]));
							hasUpdated = true;
						}

						if (material->GetNormalMapPtr()->GetRenderRef().DescriptorIndex == texture->GetRenderRef().DescriptorIndex)
						{
							material->SetNormalMap(this->Get<NEW_Asset::Texture>(textures[m_diRenderer->GetRenderAssetManager().GetDefaultTextureIndex()]));
							hasUpdated = true;
						}

						if (hasUpdated)
						{
							m_diRenderer->RegisterOrUpdateAsset(*material.get());
						}
					}
				}

				m_diRenderer->UnregisterAsset(asset);

				for (auto& [sceneID, scenePtr] : m_RegisteredScenes)
				{
					// todo Call scene function that removes it from the scene if theres anything referencing it
					scenePtr->OnAssetErased(asset); // Force recache of static objects since their components need to be reuploaded
				}

				return true;
			}

			Rendering::Renderer* m_diRenderer = nullptr;
			std::tuple<AssetContainer<AssetTypes>...> m_AssetContainer;
			std::unordered_map<Scene::SceneID, Scene::Scene*> m_RegisteredScenes;
		};
	}
}