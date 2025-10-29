#include "misc/NonCopyable.hpp"
#include "misc/NonMovable.hpp"
#include "assets/Mesh.hpp"
#include "assets/Material.hpp"
#include "assets/Texture.hpp"
#include <unordered_map>
#include <string>
#include <optional>

namespace aZero
{
	namespace AssetNew
	{
		template<typename AssetType>
		class AssetAllocator : public NonCopyable, public NonMovable
		{
		public:
			AssetAllocator():m_NextAssetID(0) {};

			AssetID m_NextAssetID;
			std::unordered_map<std::string, std::shared_ptr<AssetType>> m_AssetMap;

			void ValidateName(std::string& name)
			{
				// TODO: Recursive logic to change name if needed to make it unique
			}

			std::weak_ptr<AssetType> Create(const std::string& name)
			{
				if (m_NextAssetID == std::numeric_limits<AssetID>::max())
				{
					// TODO: Figure out exactly how to handle this case
					throw;
				}

				std::string assetName = name;
				this->ValidateName(assetName);

				AssetID newAssetID = m_NextAssetID;
				m_NextAssetID++;
				m_AssetMap[assetName] = std::shared_ptr<AssetType>(new AssetType());

				// TODO: replace with constructor
				m_AssetMap[assetName]->m_AssetID = newAssetID;
				m_AssetMap[assetName]->m_Name = assetName;
				return m_AssetMap[assetName];
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

			std::weak_ptr<AssetType> Get(const std::string& name)
			{
				if (m_AssetMap.find(name) != m_AssetMap.end())
				{
					return m_AssetMap.at(name);
				}
				return std::weak_ptr<AssetType>();
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

			std::weak_ptr<Mesh> GetMesh(const std::string& name)
			{
				return m_MeshAllocator.Get(name);
			}

			std::weak_ptr<Material> GetMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Get(name);
			}

			std::weak_ptr<Texture> GetTexture(const std::string& name)
			{
				return m_TextureAllocator.Get(name);
			}

			std::weak_ptr<Mesh> CreateMesh(const std::string& name)
			{
				return m_MeshAllocator.Create(name);
			}

			bool DeleteMesh(const std::string& name)
			{
				auto mesh = m_MeshAllocator.Get(name);
				if (!mesh.expired())
				{
					// TODO: Remove references in scenes...
					// TODO: Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*mesh.lock());
					return m_MeshAllocator.Delete(name);
				}
				return false;
			}

			std::weak_ptr<Material> CreateMaterial(const std::string& name)
			{
				return m_MaterialAllocator.Create(name);
			}

			bool DeleteMaterial(const std::string& name)
			{
				auto material = m_MaterialAllocator.Get(name);
				if (!material.expired())
				{
					// TODO: Remove references in scenes...
					// TODO: Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*material.lock());
					return m_MaterialAllocator.Delete(name);
				}
				return false;
			}

			std::weak_ptr<Texture> CreateTexture(const std::string& name)
			{
				return m_TextureAllocator.Create(name);
			}

			bool DeleteTexture(const std::string& name)
			{
				auto texture = m_TextureAllocator.Get(name);
				if (!texture.expired())
				{
					// TODO: Remove references in scenes...
					
					// TODO: Remove gpu state from renderer by calling deload... m_RenderContext.Deload(*texture.lock());
					return m_TextureAllocator.Delete(name);
				}
				return false;
			}
		};
	}
}
/*
creates assets and store with name and id keys
when material/mesh is removed
	=> remove asset from renderer using id
	when scene is rendered
		=> if batch uses material or mesh that isnt in the renderer we set default material/mesh
when texture is removed, go through all materials and set the texture that was removed to the default texture and update render state
*/