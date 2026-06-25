#pragma once
#include "render_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"
#include "misc/HelperFunctions.hpp"
#include "scene/Scene.hpp"
#include "Vertex.hpp"

namespace aZero
{
	namespace NEW_Asset
	{
		struct TextureData
		{
			struct RenderRef
			{
				uint32_t DescriptorIndex;
			};

			std::vector<uint8_t> TexelData;
			uint32_t Width, Height, NumChannels;
			DXGI_FORMAT Format;
		};

		struct MaterialData
		{
			struct RenderRef
			{
				uint32_t AlbedoTextureIndex;
				uint32_t NormalMapIndex;
			};

			std::string AlbedoTexture;
			std::string NormalMap;
		};

		static inline constexpr uint32_t g_VerticesPerMeshlet = 64;
		static inline constexpr uint32_t g_PrimitivesPerMeshlet = 84;

		struct Meshlet
		{
			uint32_t VertexOffset;
			uint32_t VertexCount;
			uint32_t PrimitiveCount;
			std::array<uint32_t, g_PrimitivesPerMeshlet> Primitives;
		};

		struct MeshletMeshData
		{
			std::vector<Meshlet> Meshlets;
			std::vector<DirectX::BoundingSphere> MeshletBounds;
			std::vector<DXM::Vector3> Positions;
			std::vector<aZero::Asset::Vertex> Vertices;
		};

		struct SubmeshData
		{
			std::string Name;
			uint32_t MeshletOffset, MeshletCount;
			DirectX::BoundingSphere Bounds;
		};

		struct MeshData
		{
			struct RenderRef
			{
				uint32_t m_MeshletGlobalOffset;
				uint32_t m_VertexGlobalOffset;
			};

			std::vector<SubmeshData> m_Submeshes;
			MeshletMeshData m_VertexData;
		};


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

				m_diRenderer->RegisterAsset(*asset);

				return asset;
			}

			template<typename AssetType>
			bool Erase(const Key& key)
			{
				AssetType* asset = this->Get<AssetType>(key);
				if (!asset)
				{
					return false;
				}

				this->EraseFromReferences(*asset);

				auto& container = std::get<AssetContainer<AssetType>>(m_AssetContainer);
				container.erase(key);
				return true;
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
				std::erase_if(m_RegisteredScenes, [&scene](const auto& sceneIter) {
					return sceneIter.first == scene.GetSceneID();
				});
			}

		private:
			template<typename AssetType>
			bool EraseFromReferences(AssetType& asset)
			{
				this->OnErase(asset);
				m_diRenderer->UnregisterAsset(asset);

				for (auto& [sceneID, scenePtr] : m_RegisteredScenes)
				{
					// todo Call scene function that removes it from the scene if theres anything referencing it
					scenePtr->OnAssetErased(asset); // Force recache of static objects since their components need to be reuploaded
				}

				return true;
			}

			// NOTE! The specialization's declaration need to be accessible in each translation unit that uses it since the default version will be ran otherwise
			// Used for custom template specialization logic inside the AssetManager when an asset is removed
			template<typename AssetType>
			void OnErase(AssetType& asset) { }

			Rendering::Renderer* m_diRenderer = nullptr;
			std::tuple<AssetContainer<AssetTypes>...> m_AssetContainer;
			std::unordered_map<Scene::SceneID, Scene::Scene*> m_RegisteredScenes;
		};
	}
}