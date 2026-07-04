#pragma once
#include "FBX_Loading.hpp"
#include "simdjson.h"
#include "misc/stb_image.h"
#include <fstream>

namespace aZero
{
	namespace Rendering { class Renderer; };

	namespace Asset
	{
		inline std::string GetMeshDirectoryPath() { return PROJECT_DIRECTORY + std::string("assets/meshes/"); }
		inline std::string GetTextureDirectoryPath() { return PROJECT_DIRECTORY + std::string("assets/textures/"); }
		inline std::string GetMaterialDirectoryPath() { return PROJECT_DIRECTORY + std::string("assets/materials/"); }

		template<typename RenderRef, typename Data>
		class RenderAssetBase
		{
			friend class aZero::Rendering::Renderer;
		public:
			RenderAssetBase() = default;
			RenderAssetBase(Data&& data)
				:m_CachedData(std::move(data)) {}
			RenderAssetBase(const Data& data)
				:m_CachedData(data) {}

			const RenderRef& GetRenderRef() const { return m_RenderRef; }
			const Data& GetCachedData() const { return m_CachedData; }
			void ClearCachedData() { m_CachedData = Data(); }
		private:
			RenderRef m_RenderRef;
			Data m_CachedData;
		};

		struct MeshletMeshData
		{
			std::vector<aZero::Asset::Meshlet> Meshlets;
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
			std::string FilePath;
			std::vector<SubmeshData> m_Submeshes;
			MeshletMeshData m_VertexData;

			MeshData() = default;
			MeshData(const FBX::FBX_Mesh& mesh)
			{
				for (const FBX::FBX_Submesh& submesh : mesh.Submeshes)
				{
					std::vector<Asset::Vertex> vertices(submesh.Vertices);
					std::vector<DXM::Vector3> positions(submesh.Positions);
					std::vector<Asset::Index> indices(submesh.Indices);
					std::vector<Asset::Meshlet> meshlets;
					std::vector<DirectX::BoundingSphere> meshletBounds;
					Asset::Meshletize(positions, vertices, indices, meshlets, meshletBounds);

					SubmeshData newSubmesh;
					newSubmesh.Name = submesh.Name;
					newSubmesh.Bounds = submesh.Bounds;
					newSubmesh.MeshletOffset = m_VertexData.Meshlets.size();
					newSubmesh.MeshletCount = meshlets.size();

					m_VertexData.Positions.insert(m_VertexData.Positions.end(), positions.begin(), positions.end());
					m_VertexData.Vertices.insert(m_VertexData.Vertices.end(), vertices.begin(), vertices.end());
					m_VertexData.Meshlets.insert(m_VertexData.Meshlets.end(), meshlets.begin(), meshlets.end());
					m_VertexData.MeshletBounds.insert(m_VertexData.MeshletBounds.end(), meshletBounds.begin(), meshletBounds.end());

					m_Submeshes.push_back(newSubmesh);
				}
			}
		};

		struct MeshRenderRef
		{
			uint32_t m_MeshletGlobalOffset = std::numeric_limits<uint32_t>::max();
			uint32_t m_VertexGlobalOffset = std::numeric_limits<uint32_t>::max();

			bool IsValid() const {
				return m_MeshletGlobalOffset != std::numeric_limits<uint32_t>::max() && m_VertexGlobalOffset != std::numeric_limits<uint32_t>::max();
			}
		};

		class Mesh : public RenderAssetBase<MeshRenderRef, MeshData>
		{
		public:
			Mesh() = default;
			Mesh(MeshData&& data)
				:RenderAssetBase(std::move(data))
			{
				const auto& cachedData = this->GetCachedData();
				for (const auto& mesh : cachedData.m_Submeshes)
				{
					m_Submeshes[m_NumSubmeshes] = mesh;
					m_NumSubmeshes++;
				}
			}

			Mesh(const MeshData& data)
				:RenderAssetBase(data)
			{
				const auto& cachedData = this->GetCachedData();
				for (const auto& mesh : cachedData.m_Submeshes)
				{
					m_Submeshes[m_NumSubmeshes] = mesh;
					m_NumSubmeshes++;
				}
			}

			std::pair<uint32_t, std::array<SubmeshData, 10>> GetSubmeshes() const { return std::make_pair(m_NumSubmeshes, m_Submeshes); } // todo Return reference to array

		private:
			std::array<SubmeshData, 10> m_Submeshes;
			uint32_t m_NumSubmeshes = 0;
		};

		struct TextureData
		{
			std::string FilePath;
			std::vector<uint8_t> TexelData;
			uint32_t Width, Height, NumChannels;
			DXGI_FORMAT Format;

			TextureData() = default;
			TextureData(const std::string& filePath, DXGI_FORMAT format) { this->Load(filePath, format); }

			bool Load(const std::string& filePath, DXGI_FORMAT format)
			{
				std::int32_t width, height, channels;
				stbi_uc* loadedImage = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
				if (!loadedImage)
				{
					DEBUG_PRINT("Failed to load file: " + filePath);
					return false;
				}

				if (channels == 4 || channels == 3)
				{
					TexelData.resize(width * height * 4);
					memcpy(TexelData.data(), loadedImage, TexelData.size());
					Width = width;
					Height = height;
					NumChannels = channels;
				}
				else
				{
					DEBUG_PRINT("Loading a texture with less than 3 channels isn't supported");
					return false;
				}

				stbi_image_free(loadedImage);
				Format = format;

				FilePath = filePath;

				return true;
			}
		};

		struct TextureRenderRef
		{
			uint32_t DescriptorIndex = std::numeric_limits<uint32_t>::max();
			bool IsValid() const {
				return DescriptorIndex != std::numeric_limits<uint32_t>::max();
			}
		};

		class Texture : public RenderAssetBase<TextureRenderRef, TextureData>
		{
		public:
			Texture() = default;
			Texture(TextureData&& data)
				:RenderAssetBase(std::move(data)) {}
			Texture(const Asset::TextureData& data)
				:RenderAssetBase(data) {}

		private:

		};

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

			MaterialData() = default;
			MaterialData(const std::string& filePath) { this->Load(filePath); }

			bool Load(const std::string& filePath)
			{
				const std::string suffix = Helper::GetPathSuffix(filePath);
				if (suffix == "json")
				{
					simdjson::ondemand::parser parser;
					auto json = simdjson::padded_string::load(filePath);
					if (json.has_value())
					{
						simdjson::ondemand::document document = parser.iterate(json);

						auto albedoJsonString = document["Albedo"].get_string();
						if (albedoJsonString.has_value()) {
							AlbedoTexture = albedoJsonString.value();
						}

						auto normalJsonString = document["Normal"].get_string();
						if (normalJsonString.has_value()) {
							NormalMap = normalJsonString.value();
						}
						FilePath = filePath;
						return true;
					}
					
				}
				return false;
			}
		};

		class Material : public RenderAssetBase<MaterialRenderRef, Asset::MaterialData>
		{
			friend Rendering::Renderer;
		public:
			Material() = default;
			Material(const Asset::MaterialData& data)
				:RenderAssetBase(data) {}
			Material(Asset::MaterialData&& data)
				:RenderAssetBase(std::move(data)) {}

			void SetAlbedo(Asset::Texture& albedo) { m_AlbedoRef = &albedo; }
			void SetNormalMap(Asset::Texture& normalMap) { m_NormalRef = &normalMap; }

			Asset::Texture* GetAlbedoPtr() const { return m_AlbedoRef; }
			Asset::Texture* GetNormalMapPtr() const { return m_NormalRef; }

		private:
			Asset::Texture* m_AlbedoRef = nullptr;
			Asset::Texture* m_NormalRef = nullptr;
		};

		template<typename Key, typename ...AssetTypes>
		class AssetManagerT
		{
		public:
			AssetManagerT() = default;
			AssetManagerT(Rendering::Renderer& diRenderer, std::string_view projectRootDirectory)
				:m_diRenderer(&diRenderer), m_ProjectRootDirectory(projectRootDirectory)
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

				if constexpr (std::is_same<AssetType, Asset::Material>::value) // Ugly, but needed for this case
				{
					Asset::Material* material = static_cast<Asset::Material*>(asset);

					const std::string& albedoName = material->GetCachedData().AlbedoTexture;
					if (!albedoName.empty()) {
						Asset::Texture* albedo = this->Get<Asset::Texture>(albedoName);
						if (!albedo) {
							Asset::TextureData texData;
							texData.Load(m_ProjectRootDirectory + "textures/" + albedoName, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
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
							texData.Load(m_ProjectRootDirectory + "textures/" + normalMapName, DXGI_FORMAT_R8G8B8A8_UNORM);
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

			/*void RegisterScene(Scene::Scene* stableScenePtr)
			{
				m_RegisteredScenes[stableScenePtr->GetSceneID()] = stableScenePtr;
			}

			void UnregisterScene(const Scene::Scene& scene)
			{
				if (auto sceneIter = m_RegisteredScenes.find(scene.GetSceneID()); sceneIter != m_RegisteredScenes.end())
				{
					m_RegisteredScenes.erase(sceneIter);
				}
			}*/

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

				m_diRenderer->UnregisterAsset(asset);

				//for (auto& [sceneID, scenePtr] : m_RegisteredScenes)
				//{
				//	// todo Call scene function that removes it from the scene if theres anything referencing it
				//	scenePtr->OnAssetErased(asset); // Force recache of static objects since their components need to be reuploaded
				//}

				return true;
			}

			Rendering::Renderer* m_diRenderer = nullptr;
			std::tuple<AssetContainer<AssetTypes>...> m_AssetContainer;
			std::string m_ProjectRootDirectory;
			//std::unordered_map<Scene::SceneID, Scene::Scene*> m_RegisteredScenes;
		};
	}
}