//#pragma once
//#include <fstream>
//#include "../../DataStructures/PackedLookupArray.h"
//
//namespace aZero
//{
//	// TODO - All assets shouldnt be returned directly. Instead the user should be provided with a handle.
//
//	/*
//	The assets are stored in a ...
//
//
//	All assets shouldnt be returned directly.
//	Instead the user should be provided with a handle.
//	A handle is a lightweight referencecounted key that can be used to get information about a cached asset. 
//	Having a reference counted handle also ensures that a cached asset cannot be deleted while referenced by something.
//	The cache should provide a function that removes all unreferenced assets.
//
//	Use examples:
//	Material asset has handles to texture assets for ex. normal and albedo textures 
//		NOTE! the material asset should also cache the descriptor indices for the textures to avoid lookup
//	A material component should have a handle to a material asset
//	A mesh sub-component should have a handle to a mesh asset
//	When saving ex. a material, the handle can be used to look into the texture asset cache to get info about the specific texture
//	
//
//	*/
//	template<typename KeyType, typename AssetType>
//	class AssetCacheBase
//	{
//	private:
//		DataStructures::PackedLookupArray<KeyType, AssetType> m_Assets;
//
//	public:
//		AssetCacheBase(int NumPerIncreaseInternal = 1)
//			:m_Assets(NumPerIncreaseInternal)
//		{
//
//		}
//
//		// TODO - Implement move and copy constructors / operators
//		AssetCacheBase(const AssetCacheBase&) = delete;
//		AssetCacheBase(AssetCacheBase&&) = delete;
//		AssetCacheBase operator=(const AssetCacheBase&) = delete;
//		AssetCacheBase operator=(AssetCacheBase&&) = delete;
//
//		AssetType& Get(const KeyType& Key)
//		{
//			return m_Assets.GetElementRef(Key);
//		}
//
//		const AssetType& Get(const KeyType& Key) const
//		{
//			return m_Assets.GetElementConstRef(Key);
//		}
//
//		void Store(const KeyType& Key, AssetType&& asset)
//		{
//			DEBUG_CHECK(!m_Assets.Exists(Key));
//			m_Assets.Add(Key, std::forward<AssetType>(asset));
//		}
//
//		void Remove(const KeyType& Key)
//		{
//			DEBUG_CHECK(m_Assets.Exists(Key));
//			m_Assets.Remove(Key);
//		}
//
//		bool Exists(const KeyType& Key) const { return m_Assets.Exists(Key); }
//	};
//}