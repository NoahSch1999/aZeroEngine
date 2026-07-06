#pragma once
#include "scene/Scene.hpp"
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

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

inline bool aZero::Scene::Scene::LoadGltf(const std::filesystem::path& path, Asset::AssetManager<std::string>& assetManager)
{
	static constexpr auto supportedExtensions =
		fastgltf::Extensions::KHR_mesh_quantization |
		fastgltf::Extensions::KHR_texture_transform |
		fastgltf::Extensions::KHR_materials_variants;

	fastgltf::Parser parser(supportedExtensions);

	constexpr auto gltfOptions =
		fastgltf::Options::DontRequireValidAssetMember |
		fastgltf::Options::AllowDouble |
		fastgltf::Options::LoadExternalBuffers |
		fastgltf::Options::LoadExternalImages |
		fastgltf::Options::GenerateMeshIndices;

	auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
	if (!bool(gltfFile)) {
		std::cerr << "Failed to open glTF file: " << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
		return false;
	}

	auto loadedAsset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
	fastgltf::Asset* asset = loadedAsset.get_if();
	if (!asset) {
		std::cerr << "Failed to load glTF file: " << fastgltf::getErrorMessage(loadedAsset.error()) << '\n';
		return false;
	}

	std::vector<Asset::MeshData> meshes(asset->meshes.size());
	for (int c = 0; c < asset->meshes.size(); c++)
	{
		const fastgltf::Mesh& mesh = asset->meshes[c];
		Asset::MeshData& meshData = meshes[c];
		meshData.FilePath = path.string();
		meshData.Name = mesh.name.c_str();
		meshData.m_Submeshes.resize(mesh.primitives.size());

		uint32_t vertexOffset = 0;
		for (int i = 0; i < mesh.primitives.size(); i++)
		{
			const auto& primitive = mesh.primitives[i];
			size_t materialIndex = primitive.materialIndex.has_value() ? primitive.materialIndex.value() : 0 /*todo Set default material index*/;

			auto& positionAccessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
			/*if (!positionAccessor.bufferViewIndex.has_value())
				continue;*/

			std::vector<DXM::Vector3> positions(positionAccessor.count);
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(*asset, positionAccessor, [&positions](fastgltf::math::fvec3 pos, std::size_t idx) {
				auto position = fastgltf::math::fvec3(pos.x(), pos.y(), pos.z());
				positions[idx] = { position.x(), position.y(), position.z() };
				});

			auto& normalAccessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
			/*if (!normalAccessor.bufferViewIndex.has_value())
				continue;*/

			std::vector<Asset::Vertex> vertexData(normalAccessor.count);
			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(*asset, normalAccessor, [&vertexData](fastgltf::math::fvec3 n, std::size_t idx) {
				const auto normal = Asset::PackNormal(Helper::EncodeNormalOctahedral({ n.x(), n.y(), n.z() }));
				std::copy(normal.begin(), normal.end(), vertexData[idx].Normal);
				});

			if (const auto* texcoord = primitive.findAttribute("TEXCOORD_0"); texcoord != primitive.attributes.end()) {
				// Tex coord
				auto& texCoordAccessor = asset->accessors[texcoord->accessorIndex];
				/*if (!texCoordAccessor.bufferViewIndex.has_value())
					continue;*/

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(*asset, texCoordAccessor, [&vertexData](fastgltf::math::fvec2 uv, std::size_t idx) {
					const auto& u = Asset::PackUV({ uv.x(), uv.y() });
					std::copy(u.begin(), u.end(), vertexData[idx].UV);
					});
				int xc = 2;
			}

			auto& indexAccessor = asset->accessors[primitive.indicesAccessor.value()];
			/*if (!indexAccessor.bufferViewIndex.has_value())
				return false;*/


			std::vector<uint32_t> indices(indexAccessor.count);
			if (indexAccessor.componentType == fastgltf::ComponentType::UnsignedByte || indexAccessor.componentType == fastgltf::ComponentType::UnsignedShort)
			{
				// Only 32bit indices are supported. With mesh shaders its not gonna matter anyways since the indices will be remapped to 8bits.
				std::vector<uint16_t> tempIndices(indexAccessor.count);
				fastgltf::copyFromAccessor<uint16_t>(*asset, indexAccessor, tempIndices.data());
				for (int j = 0; j < tempIndices.size(); j++) {
					indices[j] = tempIndices[j];
				}
			}
			else
			{
				fastgltf::copyFromAccessor<uint32_t>(*asset, indexAccessor, indices.data());
			}

			std::vector<Asset::Meshlet> meshlets;
			std::vector<DirectX::BoundingSphere> meshletBounds;
			Asset::Meshletize(positions, vertexData, indices, meshlets, meshletBounds, vertexOffset);

			Asset::SubmeshData newSubmesh;
			newSubmesh.Bounds = Helper::ComputeBoundingSphere(positions);
			newSubmesh.MeshletOffset = meshData.m_VertexData.Meshlets.size();
			newSubmesh.MeshletCount = meshlets.size();
			vertexOffset += positions.size();
			meshData.m_Submeshes[i] = newSubmesh;

			meshData.m_VertexData.Positions.insert(meshData.m_VertexData.Positions.end(), positions.begin(), positions.end());
			meshData.m_VertexData.Vertices.insert(meshData.m_VertexData.Vertices.end(), vertexData.begin(), vertexData.end());
			meshData.m_VertexData.Meshlets.insert(meshData.m_VertexData.Meshlets.end(), meshlets.begin(), meshlets.end());
			meshData.m_VertexData.MeshletBounds.insert(meshData.m_VertexData.MeshletBounds.end(), meshletBounds.begin(), meshletBounds.end());
		}
	}

	for (const auto& mesh : meshes)
	{
		assetManager.Create<Asset::Mesh>(mesh.Name, mesh);
	}

	return true;
}