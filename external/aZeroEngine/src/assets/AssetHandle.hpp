/**
 * @file AssetHandle.h
 * @brief Defines the AssetHandle class that is used to hide ownership-control from the user.
 *
 * This file declares the `AssetHandle` template class within the `aZero::Asset` namespace.
 * The handle acts as a lightweight wrapper around a `std::weak_ptr` that points to a managed asset.
 * It ensures that the referenced asset is still valid before providing access.
 *
 * @date 2025-11-04
 * @version 1.0
 */

#pragma once
#include <memory>

namespace aZero
{
	namespace Asset
	{
		template<typename T>
		class AssetAllocator;

		/**
		 * @class AssetHandle
		 * @brief A lightweight smart handle to an asset managed by the AssetAllocator and AssetManager.
		 *
		 * The `AssetHandle` class provides a safe way to access an asset without maintaining
		 * ownership. Internally, it uses a `std::weak_ptr` to reference the asset, which allows
		 * checking validity before dereferencing.
		 *
		 * @tparam AssetType Type of the asset being referenced.
		 */
		template<typename AssetType>
		class AssetHandle
		{
			/// Allow AssetAllocator to create handles with private constructor
			friend class AssetAllocator<AssetType>;
		private:
			/**
			 * @brief Constructs an AssetHandle from a weak pointer reference.
			 * @param ref A weak pointer to the managed asset.
			 *
			 * This constructor is private to ensure that only `AssetAllocator` and thus `AssetManager` can
			 * create valid handles.
			 */
			AssetHandle(std::weak_ptr<AssetType> ref)
				:m_Ref(ref){}

			/// Internal weak reference to the asset.
			std::weak_ptr<AssetType> m_Ref;
		public:
			/**
			 * @brief Default constructor.
			 *
			 * Creates an invalid (empty) handle. Use `IsValid()` to check validity before use.
			 */
			explicit AssetHandle() = default;

			/**
			 * @brief Retrieves a pointer to the asset if it is still valid.
			 *
			 * @return A pointer to the asset if valid; otherwise, returns `nullptr`.
			 *
			 * Keep in mind that this pointer shouldn't be stored for long-term use since it doesn't have ownership of the asset.
			 */
			AssetType* GetAsset() const
			{ 
				return this->IsValid() ? m_Ref.lock().get() : nullptr;
			}

			/**
			 * @brief Checks whether the handle still references a valid asset.
			 *
			 * @return `true` if the referenced asset is still alive, `false` otherwise.
			 */
			bool IsValid() const { return !m_Ref.expired(); }
		};
	}
}