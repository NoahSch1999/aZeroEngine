#pragma once
#include <vector>
#include "misc/NonCopyable.hpp"
#include "misc/NonMovable.hpp"

namespace aZero
{
	namespace Asset
	{
		using RenderAssetID = std::uint32_t;

		template<typename AssetType>
		struct RenderAssetIDArray
		{
			std::vector<RenderAssetID> m_IDs;
		};

		template<typename... Args>
		class RenderGarbageCollectorClass : public NonCopyable, public NonMovable
		{
		private:
			std::tuple<RenderAssetIDArray<Args>...> m_AssetArrays;

		public:

			RenderGarbageCollectorClass() = default;

			template<typename AssetType>
			void Collect(RenderAssetID ID)
			{
				std::get<RenderAssetIDArray<AssetType>>(m_AssetArrays).m_IDs.push_back(ID);
			}

			template<typename AssetType>
			std::vector<RenderAssetID>& GetGarbageIDs()
			{
				return std::get<RenderAssetIDArray<AssetType>>(m_AssetArrays).m_IDs;
			}
		};
	}
}