#pragma once
#include "misc/NonCopyable.hpp"
#include "misc/NonMovable.hpp"
#include "assets/Mesh.hpp"
#include "assets/Material.hpp"
#include "assets/Texture.hpp"
#include "misc/HelperFunctions.hpp"
#include <unordered_map>
#include <string>
#include <optional>

namespace aZero
{
	namespace Asset
	{
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

		class AssetManager : public NonCopyable, public NonMovable
		{
		private:
			NewAssetAllocator<Mesh> m_MeshAllocator;
			NewAssetAllocator<Material> m_MaterialAllocator;
			NewAssetAllocator<Texture> m_TextureAllocator;
		public:
			AssetManager() = default;

			Mesh* CreateMesh(const std::string& name)
			{
				return m_MeshAllocator.Create(name);
			}

			Material* CreateMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Create(name);
			}

			Texture* CreateTexture(const std::string& name)
			{
				return m_TextureAllocator.Create(name);
			}

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

			Mesh* GetMesh(const std::string& name)
			{
				return m_MeshAllocator.Get(name);
			}

			Material* GetMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Get(name);
			}

			Texture* GetTexture(const std::string& name)
			{
				return m_TextureAllocator.Get(name);
			}
		};
	}
}