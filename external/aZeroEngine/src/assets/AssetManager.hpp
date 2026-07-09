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
			:m_diRenderer(&diRenderer), m_ProjectRootDirectory(projectRootDirectory) {
			
			Asset::TextureData fallbackTextureData;
			fallbackTextureData.Name = "Fallback";
			fallbackTextureData.TexelData = { 1,0,1,1 };
			fallbackTextureData.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			fallbackTextureData.Height = 1;
			fallbackTextureData.Width = 1;
			fallbackTextureData.NumChannels = 4;
			Asset::Texture* fallbackTexture = this->Create<Asset::Texture>("Fallback", std::move(fallbackTextureData));

			Asset::TextureData fallbackNormalMapData;
			fallbackNormalMapData.Name = "FallbackNormalMap";
			fallbackNormalMapData.TexelData = { 0,0,0,1 };
			fallbackNormalMapData.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			fallbackNormalMapData.Height = 1;
			fallbackNormalMapData.Width = 1;
			fallbackNormalMapData.NumChannels = 4;
			Asset::Texture* fallbackNormalMap = this->Create<Asset::Texture>("FallbackNormalMap", std::move(fallbackNormalMapData));

			Asset::MaterialData fallbackMaterialData;
			fallbackMaterialData.Name = "Fallback";
			fallbackMaterialData.Info.AlbedoTexture = fallbackTexture;
			fallbackMaterialData.Info.NormalTexture = fallbackNormalMap;
			this->Create<Asset::Material>("Fallback", std::move(fallbackMaterialData));
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

			// todo Implement the case where the texture references for a material aren't valid
			
			//if constexpr (std::is_same<AssetType, Asset::Material>::value) // Ugly, but needed for this case
			//{
			//	Asset::Material* material = static_cast<Asset::Material*>(asset);

			//	const std::string& albedoName = material->GetCachedData().AlbedoTexture;
			//	if (!albedoName.empty()) {
			//		Asset::Texture* albedo = this->Get<Asset::Texture>(albedoName);
			//		if (!albedo) {
			//			Asset::TextureData texData;
			//			texData.Load(this->GetAssetDirectory<Asset::Texture>() + albedoName, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
			//			this->Create<Asset::Texture>(albedoName, std::move(texData));
			//			albedo = this->Get<Asset::Texture>(albedoName);
			//		}
			//		material->SetAlbedo(*albedo);
			//	}

			//	const std::string& normalMapName = material->GetCachedData().NormalMap;
			//	if (!normalMapName.empty()) {
			//		Asset::Texture* normalMap = this->Get<Asset::Texture>(normalMapName);
			//		if (!normalMap) {
			//			Asset::TextureData texData;
			//			texData.Load(this->GetAssetDirectory<Asset::Texture>() + normalMapName, DXGI_FORMAT_R8G8B8A8_UNORM);
			//			this->Create<Asset::Texture>(normalMapName, std::move(texData));
			//			normalMap = this->Get<Asset::Texture>(normalMapName);
			//		}
			//		material->SetNormalMap(*normalMap);
			//	}
			//}

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

inline std::unordered_map<uint32_t, std::string> aZero::Scene::Scene::LoadGltf_Meshes(const std::filesystem::path& path, Asset::AssetManager<std::string>& assetManager, fastgltf::Asset* asset)
{
	std::unordered_map<uint32_t, std::string> meshIndexToName;
	for (int c = 0; c < asset->meshes.size(); c++)
	{
		const fastgltf::Mesh& mesh = asset->meshes[c];
		const uint32_t numPrimitives = std::clamp((uint32_t)mesh.primitives.size(), 0u, Component::Mesh::s_MaxNumberOfSubmeshes); // Currently only supports atmost 10 submeshes
		Asset::MeshData meshData;
		meshData.FilePath = path.string();
		meshData.Name = mesh.name.c_str();
		meshData.m_Submeshes.resize(numPrimitives);

		uint32_t vertexOffset = 0;
		for (int i = 0; i < numPrimitives; i++)
		{
			const auto& primitive = mesh.primitives[i];
			size_t materialIndex = primitive.materialIndex.has_value() ? primitive.materialIndex.value() : 0 /*todo Set default material index*/;

			size_t baseColorTexcoordIndex = 0;

			if (primitive.materialIndex.has_value())
			{
				const fastgltf::Material& material = asset->materials[primitive.materialIndex.value()];
				if (material.pbrData.baseColorTexture->transform && material.pbrData.baseColorTexture->transform->texCoordIndex.has_value()) {
					baseColorTexcoordIndex = material.pbrData.baseColorTexture->transform->texCoordIndex.value();
				}
				else {
					baseColorTexcoordIndex = material.pbrData.baseColorTexture->texCoordIndex;
				}
			}

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

			const std::string texcoordAttribute = std::string("TEXCOORD_") + std::to_string(baseColorTexcoordIndex);
			if (const auto* texcoord = primitive.findAttribute(texcoordAttribute); texcoord != primitive.attributes.end()) {
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

		meshIndexToName[c] = meshData.Name;
		Asset::Mesh* m = assetManager.Create<Asset::Mesh>(meshIndexToName[c], std::move(meshData));
		m->ClearCachedData();
	}

	return meshIndexToName;
}

inline std::unordered_map<uint32_t, std::string> aZero::Scene::Scene::LoadGltf_Materials(const std::filesystem::path& path, Asset::AssetManager<std::string>& assetManager, fastgltf::Asset* asset, const std::unordered_map<uint32_t, std::string>& textureIndexToName)
{
	std::unordered_map<uint32_t, std::string> materialIndexToName;
	for (int i = 0; i < asset->materials.size(); i++)
	{
		const fastgltf::Material& material = asset->materials[i];
		materialIndexToName[i] = material.name;
		Asset::MaterialData materialData;
		materialData.Name = material.name;

		if (material.pbrData.baseColorTexture.has_value()) {
			// todo Lookup texture in asset manager and set ptr
			Asset::Texture* texture = assetManager.Get<Asset::Texture>(textureIndexToName.at(material.pbrData.baseColorTexture.value().textureIndex));
			materialData.Info.AlbedoTexture = texture;
		}
		
		if (!materialData.Info.AlbedoTexture) {
			Asset::Texture* texture = assetManager.Get<Asset::Texture>("Fallback");
			std::cout << "Fallback albedo texture used for material: " << materialData.Name << "\n";
			materialData.Info.AlbedoTexture = texture;
		}

		if (material.normalTexture.has_value()) {
			// todo Lookup texture in asset manager and set ptr
			Asset::Texture* texture = assetManager.Get<Asset::Texture>(textureIndexToName.at(material.normalTexture.value().textureIndex));
			materialData.Info.NormalTexture = texture;
		}

		if (!materialData.Info.NormalTexture) {
			Asset::Texture* texture = assetManager.Get<Asset::Texture>("FallbackNormalMap");
			std::cout << "Fallback normal map used for material: " << materialData.Name << "\n";
			materialData.Info.NormalTexture = texture;
		}

		if (material.pbrData.metallicRoughnessTexture.has_value()) {
			uint32_t textureIndex = material.pbrData.metallicRoughnessTexture.value().textureIndex;
			// todo Lookup texture in asset manager and set ptr
			materialData.Info.MetallicRoughnessTexture = nullptr;
		}
		
		materialData.Info.RoughnessFactor = material.pbrData.roughnessFactor;
		materialData.Info.MetallicFactor = material.pbrData.metallicFactor;

		materialIndexToName[i] = materialData.Name;
		Asset::Material* mat = assetManager.Create<Asset::Material>(materialIndexToName[i], std::move(materialData));
		mat->ClearCachedData();
	}

	return materialIndexToName;
}

inline std::unordered_map<uint32_t, std::string> aZero::Scene::Scene::LoadGltf_Textures(const std::filesystem::path& path, Asset::AssetManager<std::string>& assetManager, fastgltf::Asset* asset)
{
	std::unordered_map<uint32_t, std::string> textureIndexToName;

	for (int i = 0; i < asset->textures.size(); i++)
	{
		const fastgltf::Texture& texture = asset->textures[i];
		Asset::TextureData textureData;
		if (texture.imageIndex.has_value()) {
			const fastgltf::Image& image = asset->images[texture.imageIndex.value()];

			std::visit(fastgltf::visitor{
				[](const auto& arg) {},
				[&](const fastgltf::sources::URI& filePath) {
					const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());

					// todo Figure out how to handle formatting
					textureData.Load(path, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
				},
				[&](const fastgltf::sources::Array& vector) {
					textureData.LoadFromMemory(image.name.c_str(), DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, vector.bytes.data(), vector.bytes.size(), 0);
				},
				[&](const fastgltf::sources::BufferView& view) {
					auto& bufferView = asset->bufferViews[view.bufferViewIndex];
					auto& buffer = asset->buffers[bufferView.bufferIndex];
					std::visit(fastgltf::visitor{
						[](const auto& arg) {},
						[&](const fastgltf::sources::Array& vector) {
							textureData.LoadFromMemory(image.name.c_str(), DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, vector.bytes.data(), bufferView.byteLength, bufferView.byteOffset);
						}
					}, buffer.data);
				}
				}, image.data);

			if (textureData.TexelData.size()) {
				textureIndexToName[i] = textureData.Name;
				Asset::Texture* tex = assetManager.Create<Asset::Texture>(textureIndexToName[i], std::move(textureData));
				tex->ClearCachedData();
			}
		}
		else {
			throw std::runtime_error("Texture had no connected image.");
		}
	}

	return textureIndexToName;
}

inline bool aZero::Scene::Scene::LoadGltf(const std::filesystem::path& path, Asset::AssetManager<std::string>& assetManager)
{
	static constexpr auto supportedExtensions =
		fastgltf::Extensions::KHR_mesh_quantization  |
		fastgltf::Extensions::KHR_texture_transform  |
		fastgltf::Extensions::KHR_materials_variants |
		fastgltf::Extensions::KHR_lights_punctual
		;

	fastgltf::Parser parser(supportedExtensions);

	constexpr auto gltfOptions =
		fastgltf::Options::DontRequireValidAssetMember |
		fastgltf::Options::AllowDouble |
		fastgltf::Options::LoadExternalBuffers |
		fastgltf::Options::LoadExternalImages |
		fastgltf::Options::GenerateMeshIndices |
		fastgltf::Options::DecomposeNodeMatrices
		;

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

	// ------------------------------------------------------------------------------------------------------------------------------------------------
	// LOAD TEXTURES
	// ------------------------------------------------------------------------------------------------------------------------------------------------
	std::unordered_map<uint32_t, std::string> textureIndexToName = this->LoadGltf_Textures(path, assetManager, asset);

	// ------------------------------------------------------------------------------------------------------------------------------------------------
	// LOAD MESHES
	// ------------------------------------------------------------------------------------------------------------------------------------------------
	std::unordered_map<uint32_t, std::string> meshIndexToName = this->LoadGltf_Meshes(path, assetManager, asset);

	// ------------------------------------------------------------------------------------------------------------------------------------------------
	// LOAD MATERIALS
	// ------------------------------------------------------------------------------------------------------------------------------------------------
	std::unordered_map<uint32_t, std::string> materialIndexToName = this->LoadGltf_Materials(path, assetManager, asset, textureIndexToName);

	// ------------------------------------------------------------------------------------------------------------------------------------------------
	// LOAD NODES
	// ------------------------------------------------------------------------------------------------------------------------------------------------
	fastgltf::iterateSceneNodes(*asset, 0, fastgltf::math::fmat4x4(),
		[&](const fastgltf::Node& node, fastgltf::math::fmat4x4 matrix) {

			flecs::entity entity = m_World.entity(node.name.c_str());
			std::visit(fastgltf::visitor{
				[](const auto& arg) { },
				[&](const fastgltf::TRS& tf) {
					Component::Position position(tf.translation.x(), tf.translation.y(), tf.translation.z());
					Component::Rotation rotation(DXM::Quaternion(tf.rotation.x(), tf.rotation.y(), tf.rotation.z(), tf.rotation.w()).ToEuler());
					Component::Scale scale(tf.scale.x(), tf.scale.y(), tf.scale.z());
					entity.set<Component::Position>(position);
					entity.set<Component::Rotation>(rotation);
					entity.set<Component::Scale>(scale);
				},
				[&](const fastgltf::math::fmat4x4& tf) {

				}
			}, node.transform);

			if (node.meshIndex.has_value()) {
				Asset::Mesh* mesh = assetManager.Get<Asset::Mesh>(meshIndexToName[node.meshIndex.value()]);
				if (mesh) {
					Asset::Material* material = assetManager.Get<Asset::Material>(materialIndexToName[asset->meshes[node.meshIndex.value()].primitives[0].materialIndex.value()]);
					Component::Mesh meshComponent(*mesh, *material);
					for (int32_t i = 0; i < meshComponent.m_NumSubmeshes; i++)
					{
						meshComponent.SetMaterial(i, *assetManager.Get<Asset::Material>(materialIndexToName[asset->meshes[node.meshIndex.value()].primitives[i].materialIndex.value()]));
					}
					entity.set<Component::Mesh>(meshComponent);
				}
			}
			if (node.cameraIndex.has_value()) {

			}
			if (node.lightIndex.has_value()) {

			}
		});

	return true;
}