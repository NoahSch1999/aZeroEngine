#pragma once
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "misc/NonCopyable.hpp"
#include "renderer/Renderer.hpp"

namespace aZero::Asset
{
	/*
	This class is wack but whatever... this will definitely be reworked from scratch at some point... :/

	NOTE:
		REGARDING STIRNG ARGUMENTS:
			All arguments that has are called something with "filePath" require the full absolute path.
			All arguments that has are called something with "name" require only the name of the file (without a suffix).
			This is because we want to be able to load assets by an absolute path so we can dynamically parse the file correctly whether it's json, png, fbx etc...
			Names are generated based on the loaded absolute paths but are stripped of everything except the actual filename without the suffix.

		REGARDING FILE LOCATIONS:
			Meshes, textures, and materials can be loaded from anywhere.
			HOWEVER, materials that are loaded require that it's dependent texture files are already in the engine texture folder IF AND ONLY IF the textures haven't already been loaded.
	*/
	class AssetManager : public NonCopyable
	{
	public:
		AssetManager(Rendering::Renderer& diRenderer)
			:m_diRenderer(&diRenderer)
		{

		}

		// Creating
		std::optional<Asset::Material*> CreateMaterial(const std::string& name)
		{
			if (m_Materials.count(name))
			{
				DEBUG_PRINT("Material can't be created since it's already in the AssetManager.\n");
				return {};
			}
			m_Materials[name] = Asset::Material();
			return &m_Materials[name];
		}

		// Saving
		bool SaveMaterialJSON(const std::string& existingMaterialName, const std::string& outputFilename)
		{
			if (!m_Materials.count(existingMaterialName))
			{
				DEBUG_PRINT("Material can't save since it's not in the AssetManager.\n");
				return false;
			}
			return m_Materials[existingMaterialName].Save(Asset::GetMaterialDirectoryPath() + outputFilename + ".json");
		}

		// Loading
		bool LoadMesh(const std::string& filePath)
		{
			std::string meshName = Helper::GetFilenameFromPath(filePath);
			meshName = Helper::StripSuffixFromFilePath(meshName);
			if (m_Meshes.count(meshName))
			{
				DEBUG_PRINT("Mesh can't load since it's already in the AssetManager.\n");
				return false;
			}

			Asset::Mesh newMesh;
			bool loaded = newMesh.LoadFromFile(filePath);
			if (loaded)
			{
				m_Meshes[meshName] = std::move(newMesh);
				m_diRenderer->UpdateRenderState(m_Meshes[meshName]);
			}
			else
			{
				DEBUG_PRINT("Mesh can't load.\n");
			}
			return loaded;
		}

		bool LoadTexture(const std::string& filePath, DXGI_FORMAT format)
		{
			std::string textureName = Helper::GetFilenameFromPath(filePath);
			textureName = Helper::StripSuffixFromFilePath(textureName);
			if (m_Textures.count(textureName))
			{
				DEBUG_PRINT("Texture can't load since it's already in the AssetManager.\n");
				return false;
			}

			Asset::Texture newTexture;
			bool loaded = newTexture.LoadFromFile(filePath, format);
			if (loaded)
			{
				m_Textures[textureName] = std::move(newTexture);
				m_diRenderer->UpdateRenderState(m_Textures[textureName]);
			}
			else
			{
				DEBUG_PRINT("Texture can't load.\n");
			}
			return loaded;
		}

		bool LoadMaterial(const std::string& filePath)
		{
			std::string materialName = Helper::GetFilenameFromPath(filePath);
			materialName = Helper::StripSuffixFromFilePath(materialName);
			if (m_Materials.count(materialName))
			{
				DEBUG_PRINT("Material can't load since it's already in the AssetManager.\n");
				return false;
			}

			Asset::Material newMaterial;
			bool loaded = newMaterial.LoadFromFile(filePath);
			if (loaded)
			{
				std::optional<Asset::Texture*> albedo = this->GetTexture(Helper::StripSuffixFromFilePath(newMaterial.GetLoadedData().AlbedoTexture));
				std::optional<Asset::Texture*> normalMap = this->GetTexture(Helper::StripSuffixFromFilePath(newMaterial.GetLoadedData().NormalMap));

				if (!albedo.has_value())
				{
					if (!this->LoadTexture(Asset::GetTextureDirectoryPath() + newMaterial.GetLoadedData().AlbedoTexture, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)) {
						return false;
					}
				}

				if (!normalMap.has_value())
				{
					if (!this->LoadTexture(Asset::GetTextureDirectoryPath() + newMaterial.GetLoadedData().NormalMap, DXGI_FORMAT_R8G8B8A8_UNORM)) {
						return false;
					}
				}

				newMaterial.SetAlbedoTexture(this->GetTexture(Helper::StripSuffixFromFilePath(newMaterial.GetLoadedData().AlbedoTexture)).value());
				newMaterial.SetNormalMap(this->GetTexture(Helper::StripSuffixFromFilePath(newMaterial.GetLoadedData().NormalMap)).value());

				m_Materials[materialName] = std::move(newMaterial);
				m_diRenderer->UpdateRenderState(m_Materials[materialName]);
			}
			else
			{
				DEBUG_PRINT("Material can't load.\n");
			}
			return loaded;
		}

		// Getters
		std::optional<Asset::Mesh*> GetMesh(const std::string& name)
		{
			if (!m_Meshes.count(name))
			{
				return {};
			}

			return &m_Meshes[name];
		}

		std::optional<Asset::Texture*> GetTexture(const std::string& name)
		{
			if (!m_Textures.count(name))
			{
				return {};
			}

			return &m_Textures[name];
		}

		std::optional<Asset::Material*> GetMaterial(const std::string& name)
		{
			if (!m_Materials.count(name))
			{
				return {};
			}

			return &m_Materials[name];
		}

		// Removing assets
		void RemoveMesh(const std::string& name)
		{
			if (m_Meshes.count(name))
			{
				Asset::Mesh* mesh = &m_Meshes[name];

				for (auto& [id, scene] : m_DependentScenes)
				{
					scene->RemoveMeshesWith(mesh->GetRenderID());
				}

				m_diRenderer->RemoveRenderState(m_Meshes[name]);
				m_Meshes.erase(name);
			}
		}

		void RemoveTexture(const std::string& name)
		{
			if (m_Textures.count(name))
			{
				Asset::Texture* texture = &m_Textures[name];

				// THIS IS HORRID!!!! :P
				for (auto& [name, material] : m_Materials)
				{
					bool updated = false;
					if (material.GetAlbedoTexture() == texture)
					{
						material.SetAlbedoTexture(nullptr);
						updated = true;
					}
					
					if (material.GetNormalMap() == texture)
					{
						material.SetNormalMap(nullptr);
						updated = true;
					}

					if (updated) 
					{
						m_diRenderer->UpdateRenderState(material);
					}
				}

				m_diRenderer->RemoveRenderState(m_Textures[name]);
				m_Textures.erase(name);
			}
		}

		void RemoveMaterial(const std::string& name)
		{
			if (m_Materials.count(name))
			{
				Asset::Material* material= &m_Materials[name];

				for (auto& [id, scene] : m_DependentScenes)
				{
					scene->RemoveMeshesWithMaterial(material->GetRenderID());
				}

				m_diRenderer->RemoveRenderState(m_Materials[name]);
				m_Materials.erase(name);
			}
		}

		// Registering scenes
		void RegisterScene(Scene::Scene& scene)
		{
			if (!m_DependentScenes.count(scene.GetSceneID()))
			{
				m_DependentScenes[scene.GetSceneID()] = &scene;
			}
		}

		void UnregisterScene(const Scene::Scene& scene)
		{
			if (m_DependentScenes.count(scene.GetSceneID()))
			{
				m_DependentScenes.erase(scene.GetSceneID());
			}
		}
		//

		const std::unordered_map<std::string, Asset::Mesh>& GetAllMeshes() const { return m_Meshes; }

	private:
		Rendering::Renderer* m_diRenderer = nullptr;
		std::unordered_map<std::string, Asset::Mesh> m_Meshes;

		std::unordered_map<std::string, Asset::Texture> m_Textures;

		std::unordered_map<std::string, Asset::Material> m_Materials;

		std::unordered_map<Scene::SceneID, Scene::Scene*> m_DependentScenes;
	};
}