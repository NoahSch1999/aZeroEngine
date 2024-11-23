#pragma once
#include "Core/D3D12Core.h"
#include "Core/AssetTypes/MaterialAsset.h"
#include <unordered_map>
#include <queue>

namespace aZero
{
	namespace Asset
	{
		template<typename MaterialType>
		class MaterialCache
		{
		private:
			std::unordered_map<std::string, std::shared_ptr<MaterialType>> m_Materials;
			std::queue<int> m_FreeMaterialIDs;
			int m_NextMaterialID = 1;
		public:

			MaterialCache() = default;

			std::shared_ptr<MaterialType> CacheMaterial(const MaterialType& Material)
			{
				const std::string& Name = Material.GetName();
				if (m_Materials.count(Name) == 0)
				{
					int MaterialID = 0;
					if (m_FreeMaterialIDs.empty())
					{
						MaterialID = m_NextMaterialID;
						m_NextMaterialID++;
					}
					else
					{
						MaterialID = m_FreeMaterialIDs.front();
						m_FreeMaterialIDs.pop();
					}

					MaterialType NewMaterial(Material);
					NewMaterial.SetID(MaterialID);
					m_Materials.emplace(Name, std::make_unique<MaterialType>(NewMaterial));
				}
				else
				{
					// NOTE: Handle if material with name already exists
					throw;
				}

				return nullptr;
			}

			void RemoveMaterial(const std::string& Name)
			{
				if (m_Materials.count(Name) != 0)
				{
					m_FreeMaterialIDs.push(m_Materials.at(Name).GetID());
					m_Materials.erase(Name);
				}
			}

			std::shared_ptr<MaterialType> GetMaterialRef(const std::string& Name)
			{
				return m_Materials.count(Name) > 0 ? m_Materials.at(Name) : nullptr;
			}
		};
	}
}