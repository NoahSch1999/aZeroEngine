#pragma once
#include "RenderAssetData.hpp"

namespace aZero
{
	namespace Asset
	{
		using Mesh = RenderAsset<MeshData, MeshEntry>;
		using Texture = RenderAsset<TextureData, TextureEntry>;
		using Material = RenderAsset<MaterialData, MaterialEntry>;

		// Macro :((((
		#define RENDER_ASSET_TYPES Mesh, Texture, Material

		using RenderGarbageCollector = RenderGarbageCollectorClass<RENDER_ASSET_TYPES>;

		template<typename AssetType, typename GPUHandleType>
		class RenderAsset : public NonCopyable
		{
		private:
			RenderGarbageCollector* m_GBCollector;
			RenderAssetID m_ID = std::numeric_limits<RenderAssetID>::max();
			AssetType m_Data;
		
		public:
			using GPUHandle = GPUHandleType;

			RenderAsset(RenderGarbageCollector& GBCollector, RenderAssetID ID)
				:m_GBCollector(&GBCollector), m_ID(ID) { }

			RenderAsset(RenderAsset<AssetType, GPUHandleType>&& Other) noexcept
			{
				m_ID = Other.m_ID;
				m_GBCollector = Other.m_GBCollector;
				m_Data = Other.m_Data;

				Other.m_GBCollector = nullptr;
			}

			RenderAsset& operator=(RenderAsset<AssetType, GPUHandleType>&& Other)
			{
				if (this != &Other)
				{
					m_ID = Other.m_ID;
					m_GBCollector = Other.m_GBCollector;
					m_Data = Other.m_Data;

					Other.m_GBCollector = nullptr;
				}
				return *this;
			}
		
			~RenderAsset()
			{
				if (m_GBCollector)
				{
					m_GBCollector->Collect<RenderAsset<AssetType, GPUHandleType>>(m_ID);
				}
			}

			template<typename ...Args>
			bool LoadFromFile(Args&&...InArgs)
			{
				return m_Data.LoadFromFile(InArgs...);
			}

			RenderAssetID GetRenderID() const { return m_ID; }
			void SetData(AssetType&& Data) { m_Data = std::move(Data); }
			const AssetType& GetData() const { return m_Data; }
		};
	}
}