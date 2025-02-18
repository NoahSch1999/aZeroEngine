#pragma once
#include "RenderAsset.hpp"
#include "misc/FreelistAllocator.hpp"

#include <optional>

namespace aZero
{
	namespace Asset
	{
		template<typename... Args>
		class RenderAssetManagerClass
		{
		private:
			template<typename AssetType>
			struct AssetTypeEntry
			{
				RenderAssetID m_CurrentID = 0;
				std::unordered_map<RenderAssetID, typename AssetType::GPUHandle> m_Handles;
			};
			std::tuple<AssetTypeEntry<Args>...> m_AssetTypeEntries;

			RenderGarbageCollector m_GBCollector;

			template<typename AssetType>
			AssetTypeEntry<AssetType>& GetAssetEntry()
			{
				return std::get<AssetTypeEntry<AssetType>>(m_AssetTypeEntries);
			}

		public:
			RenderAssetManagerClass() = default;

			template<typename AssetType>
			bool HasRenderHandle(const AssetType& Asset)
			{
				return this->GetAssetEntry<AssetType>().m_Handles.count(Asset.GetRenderID());
			}

			template<typename AssetType>
			void AddRenderHandle(const AssetType& Asset, typename AssetType::GPUHandle&& Handle)
			{
				this->GetAssetEntry<AssetType>().m_Handles[Asset.GetRenderID()] = std::forward<typename AssetType::GPUHandle>(Handle);
			}

			template<typename AssetType>
			void RemoveRenderHandle(const AssetType& Asset)
			{
				if (this->HasRenderHandle(Asset))
				{
					this->GetAssetEntry<AssetType>().m_Handles.erase(Asset.GetRenderID());
				}
			}

			template<typename AssetType>
			void RemoveRenderHandle(RenderAssetID ID)
			{
				if (this->GetAssetEntry<AssetType>().m_Handles.count(ID))
				{
					this->GetAssetEntry<AssetType>().m_Handles.erase(ID);
				}
			}

			template<typename AssetType>
			std::optional<typename AssetType::GPUHandle*> GetRenderHandle(const AssetType& Asset)
			{
				if (!this->HasRenderHandle(Asset))
				{
					return {};
				}

				return &this->GetAssetEntry<AssetType>().m_Handles.at(Asset.GetRenderID());
			}

			template<typename AssetType>
			std::shared_ptr<AssetType> CreateAsset()
			{
				const RenderAssetID NewID = std::get<AssetTypeEntry<AssetType>>(m_AssetTypeEntries).m_CurrentID;
				std::get<AssetTypeEntry<AssetType>>(m_AssetTypeEntries).m_CurrentID++;
				return std::make_shared<AssetType>(m_GBCollector, NewID);
			}

			template<typename AssetType>
			void ClearGarbage()
			{
				std::vector<RenderAssetID>& IDs = m_GBCollector.GetGarbageIDs<AssetType>();
				for (const RenderAssetID& ID : IDs)
				{
					this->RemoveRenderHandle<AssetType>(ID);
				}

				m_GBCollector.GetGarbageIDs<AssetType>().clear();
			}
		};

		using RenderAssetManager = RenderAssetManagerClass<RENDER_ASSET_TYPES>;
	}
}