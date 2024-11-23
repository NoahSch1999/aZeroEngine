#pragma once
#include <optional>

#include "ResourceRecycler.h"

namespace aZero
{
	namespace D3D12
	{
		class GPUBuffer;
		class GPUTexture;

		struct ResourceTransitionBundles
		{
			D3D12_RESOURCE_STATES StateBefore, StateAfter;
			ID3D12Resource* ResourcePtr;
		};

		void TransitionResources(ID3D12GraphicsCommandList* CmdList, const std::vector<ResourceTransitionBundles>& Bundles);

		class GPUResourceBase
		{
		private:
			ResourceRecycler* m_ResourceRecycler = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
			void* m_MappedPtr = nullptr;

		private:
			void MoveOp(GPUResourceBase& other)
			{
				m_Resource = other.m_Resource;
				m_ResourceRecycler = other.m_ResourceRecycler;
				m_MappedPtr = other.m_MappedPtr;

				other.m_MappedPtr = nullptr;
				other.m_Resource = nullptr;
				other.m_ResourceRecycler = nullptr;
			}
			
		public:
			GPUResourceBase() = default;

			GPUResourceBase(
				D3D12_RESOURCE_DESC ResourceDesc,
				D3D12_HEAP_PROPERTIES HeapProperties, 
				D3D12_HEAP_FLAGS HeapFlags, 
				D3D12_RESOURCE_STATES InitialResourceState,
				std::optional<D3D12_CLEAR_VALUE> OptOptimizedClearValue,
				std::optional<ResourceRecycler*> OptResourceRecycler
			)
			{
				this->Init(ResourceDesc, HeapProperties, HeapFlags, InitialResourceState, OptOptimizedClearValue, OptResourceRecycler);
			}

			GPUResourceBase(GPUResourceBase&& other) noexcept
			{
				MoveOp(other);
			}

			GPUResourceBase& operator=(GPUResourceBase&& other) noexcept
			{
				if (this != &other)
				{
					MoveOp(other);
				}

				return *this;
			}


			~GPUResourceBase()
			{
				if (m_Resource)
				{
					if (m_ResourceRecycler)
					{
						m_ResourceRecycler->RecycleResource(m_Resource);
					}
				}
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> ExistingResource, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				if (m_Resource)
				{
					if (m_ResourceRecycler)
					{
						m_ResourceRecycler->RecycleResource(m_Resource);
					}
				}

				m_Resource = ExistingResource;
				m_ResourceRecycler = OptResourceRecycler.has_value() ? OptResourceRecycler.value() : nullptr;
			}

			void Init(
				D3D12_RESOURCE_DESC ResourceDesc,
				D3D12_HEAP_PROPERTIES HeapProperties,
				D3D12_HEAP_FLAGS HeapFlags,
				D3D12_RESOURCE_STATES InitialResourceState,
				std::optional<D3D12_CLEAR_VALUE> OptOptimizedClearValue,
				std::optional<ResourceRecycler*> OptResourceRecycler
			)
			{
				if (m_Resource)
				{
					if (m_ResourceRecycler)
					{
						m_ResourceRecycler->RecycleResource(m_Resource);
					}

					m_Resource = nullptr;
				}

				m_ResourceRecycler = OptResourceRecycler.has_value() ? OptResourceRecycler.value() : nullptr;

				const HRESULT HResult = gDevice->CreateCommittedResource(
					&HeapProperties,
					D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
					&ResourceDesc,
					InitialResourceState,
					OptOptimizedClearValue.has_value() ? &OptOptimizedClearValue.value() : nullptr,
					IID_PPV_ARGS(m_Resource.GetAddressOf()));

				if (FAILED(HResult))
				{
					throw;
				}

				if (HeapProperties.Type == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
					|| HeapProperties.Type == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK)
				{
					m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedPtr));
				}
			}

			ID3D12Resource* GetResource() const { return m_Resource.Get(); }
			Microsoft::WRL::ComPtr<ID3D12Resource> GetResourceComPtr() const { return m_Resource; }
			void* GetMappedPtr() const { return m_MappedPtr; }
			D3D12::ResourceRecycler* GetResourceRecyclerPtr() { return m_ResourceRecycler; }
		};
	}
}