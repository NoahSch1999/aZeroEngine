/**
 * @file AssetManager.h
 * @brief Declares the AssetAllocator and AssetManager classes for asset management.
 *
 * This file provides the implementation of the asset management interface. 
 * It defines the `AssetAllocator` template class, which handles allocation,
 * tracking, and lifetime management of individual asset types, and the `AssetManager`
 * class, which serves as a centralized interface for creating, retrieving, and deleting
 * assets such as meshes, materials, and textures.
 *
 * @date 2025-11-04
 * @version 1.0
 */

#pragma once
#include "misc/NonCopyable.hpp"
#include "misc/NonMovable.hpp"
#include "assets/Mesh.hpp"
#include "assets/Material.hpp"
#include "assets/Texture.hpp"
#include "assets/AssetHandle.hpp"
#include <unordered_map>
#include <string>
#include <optional>

namespace aZero
{
	namespace Asset
	{
		/**
		 * @class AssetAllocator
		 * @brief Template class responsible for creating, managing, and retrieving assets.
		 *
		 * The `AssetAllocator` class manages instances of a specific asset type. It ensures
		 * that each asset is uniquely named, maintains reference-counted ownership using
		 * `std::shared_ptr`, and provides `AssetHandle` wrappers for safe access.
		 *
		 * @tparam AssetType Type of asset this allocator manages (e.g., Mesh, Material, Texture).
		 */
		template<typename AssetType>
		class AssetAllocator : public NonCopyable, public NonMovable
		{
		public:
			AssetAllocator():m_NextAssetID(0) {}

			AssetID m_NextAssetID;
			std::unordered_map<std::string, std::shared_ptr<AssetType>> m_AssetMap;

			AssetHandle<AssetType> Create(const std::string& name)
			{
				if (m_NextAssetID == std::numeric_limits<AssetID>::max())
				{
					// todo Figure out exactly how to handle this case
					throw;
				}

				std::string assetName = Helper::HandleNameCollision(name, m_AssetMap);
				AssetID newAssetID = m_NextAssetID;
				m_NextAssetID++;
				m_AssetMap[assetName] = std::shared_ptr<AssetType>(new AssetType(newAssetID, assetName));
				return AssetHandle<AssetType>(m_AssetMap[assetName]);
			}

			bool Delete(const std::string& name)
			{
				if (m_AssetMap.find(name) != m_AssetMap.end())
				{
					m_AssetMap.erase(name);
					return true;
				}
				return false;
			}

			AssetHandle<AssetType> Get(const std::string& name) const
			{
				if (auto iter = m_AssetMap.find(name); iter != m_AssetMap.end())
				{
					return AssetHandle<AssetType>(iter->second);
				}
				return AssetHandle<AssetType>();
			}

			bool Exists(const std::string& name) const
			{
				if (m_AssetMap.find(name) != m_AssetMap.end())
				{
					return true;
				}
				return false;
			}
		};

		template<typename AssetType>
		class NewAssetAllocator : public NonCopyable, public NonMovable
		{
		public:
			NewAssetAllocator() = default;

			AssetID m_NextAssetID = 0;
			std::unordered_map<std::string, std::unique_ptr<AssetType>> m_AssetMap;

			AssetType* Create(const std::string& name)
			{
				if (m_NextAssetID == std::numeric_limits<AssetID>::max())
				{
					// todo Figure out exactly how to handle this case
					throw;
				}

				std::string assetName = Helper::HandleNameCollision(name, m_AssetMap);
				AssetID newAssetID = m_NextAssetID;
				m_NextAssetID++;
				m_AssetMap.emplace(assetName, std::unique_ptr<AssetType>(new AssetType(newAssetID, assetName)));
				return m_AssetMap[assetName].get();
			}

			bool Delete(const std::string& name)
			{
				if (m_AssetMap.contains(name))
				{
					m_AssetMap.erase(name);
					return true;
				}
				return false;
			}

			AssetType* Get(const std::string& name) const
			{
				if (auto iter = m_AssetMap.find(name); iter != m_AssetMap.end())
				{
					return iter->second.get();
				}
				return nullptr;
			}

			bool Exists(const std::string& name) const
			{
				if (m_AssetMap.contains(name))
				{
					return true;
				}
				return false;
			}
		};

		/**
		 * @class AssetManager
		 * @brief Centralized manager for all asset types in the engine.
		 *
		 * The `AssetManager` acts as a unified interface for managing different asset categories.
		 * It provides creation, retrieval, and deletion methods for meshes, materials, and textures,
		 * and internally delegates management to specialized `AssetAllocator` instances.
		 */
		class NewAssetManager : public NonCopyable, public NonMovable
		{
		private:
			NewAssetAllocator<NewMesh> m_MeshAllocator;
			NewAssetAllocator<NewMaterial> m_MaterialAllocator;
			NewAssetAllocator<NewTexture> m_TextureAllocator;
		public:
			NewAssetManager() = default;

			/**
			* @brief Creates a new mesh asset and returns a non-owning AssetHandle to it
			*
			* @param name Desired name for the new mesh asset.
			* @return An `AssetHandle<Mesh>` referencing the newly created mesh.
			*
			* If a mesh with the same name already exists, a unique name is automatically
			* generated by the allocator to avoid conflicts.
			*/
			NewMesh* CreateMesh(const std::string& name)
			{
				return m_MeshAllocator.Create(name);
			}

			NewMaterial* CreateMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Create(name);
			}

			NewTexture* CreateTexture(const std::string& name)
			{
				return m_TextureAllocator.Create(name);
			}

			/**
			* @brief Deletes an existing mesh asset from the AssetManager and Renderer and returns whether or not it was deleted
			*
			* @param name Name of the mesh to delete.
			* @return `true` if the mesh was found and successfully deleted; otherwise, `false`.
			*
			* This function removes the mesh from memory and marks it for deallocation.
			* Future calls to `GetMesh()` with the same name will return an invalid handle.
			*/
			bool DeleteMesh(const std::string& name)
			{
				if (m_MeshAllocator.Exists(name))
				{
					// todo Remove references in scenes...
					// todo Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*mesh.lock());
					return m_MeshAllocator.Delete(name);
				}
				return false;
			}

			/**
			* @brief Deletes an existing material asset from the AssetManager and Renderer and returns whether or not it was deleted
			*
			* @param name Name of the material to delete.
			* @return `true` if the material was found and successfully deleted; otherwise, `false`.
			*
			* This function removes the material from memory and marks it for deallocation.
			* Future calls to `GetMaterial()` with the same name will return an invalid handle.
			*/
			bool DeleteMaterial(const std::string& name)
			{
				if (m_MaterialAllocator.Exists(name))
				{
					// todo Remove references in scenes...
					// todo Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*material.lock());
					return m_MaterialAllocator.Delete(name);
				}
				return false;
			}

			/**
			* @brief Deletes an existing texture asset from the AssetManager and Renderer and returns whether or not it was deleted
			*
			* @param name Name of the texture to delete.
			* @return `true` if the texture was found and successfully deleted; otherwise, `false`.
			*
			* This function removes the texture from memory and marks it for deallocation.
			* Future calls to `GetTexture()` with the same name will return an invalid handle.
			*/
			bool DeleteTexture(const std::string& name)
			{
				if (m_TextureAllocator.Exists(name))
				{
					// todo Remove references in scenes...
					
					// todo Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*texture.lock());
					return m_TextureAllocator.Delete(name);
				}
				return false;
			}

			/**
			* @brief Retrieves a mesh AssetHandle by name.
			*
			* @param name The name of the mesh to retrieve.
			* @return An `AssetHandle<Mesh>` referencing the mesh if it exists; otherwise, an invalid handle.
			*
			* This function provides non-owning access to an existing mesh asset managed
			* by the internal mesh allocator.
			*/
			NewMesh* GetMesh(const std::string& name)
			{
				return m_MeshAllocator.Get(name);
			}

			/**
			* @brief Retrieves a material AssetHandle by name.
			*
			* @param name The name of the material to retrieve.
			* @return An `AssetHandle<Material>` referencing the material if it exists; otherwise, an invalid handle.
			*
			* This function provides non-owning access to an existing material asset managed
			* by the internal material allocator.
			*/
			NewMaterial* GetMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Get(name);
			}

			/**
			* @brief Retrieves a texture AssetHandle by name.
			*
			* @param name The name of the texture to retrieve.
			* @return An `AssetHandle<Texture>` referencing the texture if it exists; otherwise, an invalid handle.
			*
			* This function provides non-owning access to an existing texture asset managed
			* by the internal texture allocator.
			*/
			NewTexture* GetTexture(const std::string& name)
			{
				return m_TextureAllocator.Get(name);
			}
		};

		class AssetManager : public NonCopyable, public NonMovable
		{
		private:
			AssetAllocator<Mesh> m_MeshAllocator;
			AssetAllocator<Material> m_MaterialAllocator;
			AssetAllocator<Texture> m_TextureAllocator;
		public:
			AssetManager() = default;

			/**
			* @brief Creates a new mesh asset and returns a non-owning AssetHandle to it
			*
			* @param name Desired name for the new mesh asset.
			* @return An `AssetHandle<Mesh>` referencing the newly created mesh.
			*
			* If a mesh with the same name already exists, a unique name is automatically
			* generated by the allocator to avoid conflicts.
			*/
			AssetHandle<Mesh> CreateMesh(const std::string& name)
			{
				return m_MeshAllocator.Create(name);
			}

			/**
			* @brief Creates a new material asset and returns a non-owning AssetHandle to it
			*
			* @param name Desired name for the new material asset.
			* @return An `AssetHandle<Material>` referencing the newly created material.
			*
			* If a material with the same name already exists, a unique name is automatically
			* generated by the allocator to avoid conflicts.
			*/
			AssetHandle<Material> CreateMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Create(name);
			}

			/**
			* @brief Creates a new texture asset and returns a non-owning AssetHandle to it
			*
			* @param name Desired name for the new texture asset.
			* @return An `AssetHandle<Texture>` referencing the newly created texture.
			*
			* If a texture with the same name already exists, a unique name is automatically
			* generated by the allocator to avoid conflicts.
			*/
			AssetHandle<Texture> CreateTexture(const std::string& name)
			{
				return m_TextureAllocator.Create(name);
			}

			/**
			* @brief Deletes an existing mesh asset from the AssetManager and Renderer and returns whether or not it was deleted
			*
			* @param name Name of the mesh to delete.
			* @return `true` if the mesh was found and successfully deleted; otherwise, `false`.
			*
			* This function removes the mesh from memory and marks it for deallocation.
			* Future calls to `GetMesh()` with the same name will return an invalid handle.
			*/
			bool DeleteMesh(const std::string& name)
			{
				auto mesh = m_MeshAllocator.Get(name);
				if (mesh.IsValid())
				{
					// todo Remove references in scenes...
					// todo Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*mesh.lock());
					return m_MeshAllocator.Delete(name);
				}
				return false;
			}

			/**
			* @brief Deletes an existing material asset from the AssetManager and Renderer and returns whether or not it was deleted
			*
			* @param name Name of the material to delete.
			* @return `true` if the material was found and successfully deleted; otherwise, `false`.
			*
			* This function removes the material from memory and marks it for deallocation.
			* Future calls to `GetMaterial()` with the same name will return an invalid handle.
			*/
			bool DeleteMaterial(const std::string& name)
			{
				auto material = m_MaterialAllocator.Get(name);
				if (material.IsValid())
				{
					// todo Remove references in scenes...
					// todo Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*material.lock());
					return m_MaterialAllocator.Delete(name);
				}
				return false;
			}

			/**
			* @brief Deletes an existing texture asset from the AssetManager and Renderer and returns whether or not it was deleted
			*
			* @param name Name of the texture to delete.
			* @return `true` if the texture was found and successfully deleted; otherwise, `false`.
			*
			* This function removes the texture from memory and marks it for deallocation.
			* Future calls to `GetTexture()` with the same name will return an invalid handle.
			*/
			bool DeleteTexture(const std::string& name)
			{
				auto texture = m_TextureAllocator.Get(name);
				if (texture.IsValid())
				{
					// todo Remove references in scenes...

					// todo Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*texture.lock());
					return m_TextureAllocator.Delete(name);
				}
				return false;
			}

			/**
			* @brief Retrieves a mesh AssetHandle by name.
			*
			* @param name The name of the mesh to retrieve.
			* @return An `AssetHandle<Mesh>` referencing the mesh if it exists; otherwise, an invalid handle.
			*
			* This function provides non-owning access to an existing mesh asset managed
			* by the internal mesh allocator.
			*/
			AssetHandle<Mesh> GetMesh(const std::string& name)
			{
				return m_MeshAllocator.Get(name);
			}

			/**
			* @brief Retrieves a material AssetHandle by name.
			*
			* @param name The name of the material to retrieve.
			* @return An `AssetHandle<Material>` referencing the material if it exists; otherwise, an invalid handle.
			*
			* This function provides non-owning access to an existing material asset managed
			* by the internal material allocator.
			*/
			AssetHandle<Material> GetMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Get(name);
			}

			/**
			* @brief Retrieves a texture AssetHandle by name.
			*
			* @param name The name of the texture to retrieve.
			* @return An `AssetHandle<Texture>` referencing the texture if it exists; otherwise, an invalid handle.
			*
			* This function provides non-owning access to an existing texture asset managed
			* by the internal texture allocator.
			*/
			AssetHandle<Texture> GetTexture(const std::string& name)
			{
				return m_TextureAllocator.Get(name);
			}
		};
	}
}