#pragma once
#include <optional>

#include "ResourceRecycler.h"

namespace aZero
{
	namespace D3D12
	{
		struct ResourceTransitionBundles
		{
			D3D12_RESOURCE_STATES StateBefore, StateAfter;
			ID3D12Resource* ResourcePtr;
		};

		void TransitionResources(ID3D12GraphicsCommandList* CmdList, const std::vector<ResourceTransitionBundles>& Bundles);

		class GPUResource
		{
		public:
			enum RWPROPERTY { GPUONLY = 1, CPUWRITE = 2, CPUREAD = 3 };

		private:
			D3D12::ResourceRecycler* m_ResourceRecycler = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
			void* m_MappedPtr = nullptr;

		protected:

			GPUResource(
				ID3D12Device* Device, 
				RWPROPERTY RWProperty,
				const D3D12_RESOURCE_DESC& Desc, 
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{},
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{}
			)
			{
				this->Init(Device, RWProperty, Desc, InitialState, OptClearValue, OptResourceRecycler);
			}

			void Init(
				ID3D12Device* Device,
				RWPROPERTY RWProperty,
				const D3D12_RESOURCE_DESC& Desc,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{},
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{}
			)
			{
				D3D12_HEAP_PROPERTIES HeapProperties;
				ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
				HeapProperties.Type = (D3D12_HEAP_TYPE)RWProperty;

				const D3D12_CLEAR_VALUE* ClearValue = OptClearValue.has_value() ? &OptClearValue.value() : nullptr;

				const HRESULT Res = Device->CreateCommittedResource(
					&HeapProperties,
					D3D12_HEAP_FLAG_NONE,
					&Desc,
					InitialState,
					ClearValue,
					IID_PPV_ARGS(m_Resource.GetAddressOf()));

				if (FAILED(Res))
				{
					throw std::invalid_argument("GPUResource::Init() => Failed to create commited resource");
				}

				if (RWProperty == RWPROPERTY::CPUWRITE || RWProperty == RWPROPERTY::CPUREAD)
				{
					m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedPtr));
				}

				m_ResourceRecycler = OptResourceRecycler.has_value() ? OptResourceRecycler.value() : nullptr;
			}

			GPUResource(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(InResource, OptResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				m_Resource = InResource;
				m_ResourceRecycler = OptResourceRecycler.has_value() ? OptResourceRecycler.value() : nullptr;

				D3D12_HEAP_PROPERTIES HeapProperties;
				D3D12_HEAP_FLAGS HeapFlags;
				const HRESULT Res = InResource->GetHeapProperties(&HeapProperties, &HeapFlags);
				if (FAILED(Res))
				{
					throw std::invalid_argument("GPUResource::Init() => Failed to create commited resource");
				}

				if (HeapProperties.Type == D3D12_HEAP_TYPE_UPLOAD || HeapProperties.Type == D3D12_HEAP_TYPE_READBACK)
				{
					m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedPtr));
				}
			}

		public:
			GPUResource() = default;

			~GPUResource()
			{
				if (m_ResourceRecycler)
				{
					m_ResourceRecycler->RecycleResource(m_Resource);
				}
			}

			GPUResource(const GPUResource& Other) = delete;
			GPUResource& operator=(const GPUResource& Other) = delete;

			GPUResource(GPUResource&& Other) noexcept
			{
				m_Resource = Other.m_Resource;
				m_ResourceRecycler = Other.m_ResourceRecycler;
				m_MappedPtr = Other.m_MappedPtr;

				Other.m_Resource = nullptr;
				Other.m_ResourceRecycler = nullptr;
				Other.m_MappedPtr = nullptr;
			}

			GPUResource& operator=(GPUResource&& Other) noexcept
			{
				if (this != &Other)
				{
					m_Resource = Other.m_Resource;
					m_ResourceRecycler = Other.m_ResourceRecycler;
					m_MappedPtr = Other.m_MappedPtr;

					Other.m_Resource = nullptr;
					Other.m_ResourceRecycler = nullptr;
					Other.m_MappedPtr = nullptr;
				}
				return *this;
			}

			ID3D12Resource* GetResource() const { return m_Resource.Get(); }
			D3D12::ResourceRecycler* GetResourceRecycler() const { return m_ResourceRecycler; }
			void* GetCPUAccessibleMemory() const { return m_MappedPtr; }
		};

		class GPUBuffer : public GPUResource
		{
		public:
			GPUBuffer() = default;

			GPUBuffer(
				ID3D12Device* Device,
				RWPROPERTY RWProperty,
				uint64_t NumBytes,
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{}
			) 
			{ 
				this->Init(Device, RWProperty, NumBytes, OptResourceRecycler);
			}

			void Init(
				ID3D12Device* Device,
				RWPROPERTY RWProperty,
				uint64_t NumBytes,
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{}
			)
			{
				D3D12_RESOURCE_DESC Desc;
				ZeroMemory(&Desc, sizeof(D3D12_RESOURCE_DESC));
				Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				Desc.MipLevels = 1;
				Desc.DepthOrArraySize = 1;
				Desc.Height = 1;
				Desc.Width = NumBytes;
				Desc.SampleDesc.Count = 1;
				Desc.SampleDesc.Quality = 0;
				Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
				Desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				Desc.Format = DXGI_FORMAT_UNKNOWN;

				GPUResource::Init(Device, RWProperty, Desc, D3D12_RESOURCE_STATE_COMMON, std::optional<D3D12_CLEAR_VALUE>{}, OptResourceRecycler);
			}

			GPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(InResource, OptResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				GPUResource::Init(InResource, OptResourceRecycler);
			}
		};

		class GPUTexture : public GPUResource
		{
		public:
			GPUTexture() = default;

			GPUTexture(
				ID3D12Device* Device,
				const DXM::Vector3& Dimensions,
				DXGI_FORMAT Format,
				D3D12_RESOURCE_FLAGS Flags,
				uint32_t MipLevels,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{},
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{}
			)
			{
				this->Init(Device, Dimensions, Format, Flags, MipLevels, InitialState, OptClearValue, OptResourceRecycler);
			}

			void Init(
				ID3D12Device* Device,
				const DXM::Vector3& Dimensions,
				DXGI_FORMAT Format,
				D3D12_RESOURCE_FLAGS Flags,
				uint32_t MipLevels,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{},
				std::optional<D3D12::ResourceRecycler*> OptResourceRecycler = std::optional<D3D12::ResourceRecycler*>{}
			)
			{
				D3D12_RESOURCE_DESC Desc;
				ZeroMemory(&Desc, sizeof(D3D12_RESOURCE_DESC));

				if (Dimensions.y > 0)
				{
					if (Dimensions.z > 1)
					{
						Desc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D;
					}
					else
					{
						Desc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					}
				}
				else
				{
					Desc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D;
				}

				Desc.Format = Format;
				Desc.Alignment = 0;
				Desc.Width = Dimensions.x;
				Desc.Height = Dimensions.y;
				Desc.DepthOrArraySize = Dimensions.z;
				Desc.MipLevels = MipLevels;
				Desc.SampleDesc.Count = 1;
				Desc.SampleDesc.Quality = 0;
				Desc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
				Desc.Flags = Flags;

				GPUResource::Init(Device, RWPROPERTY::GPUONLY, Desc, InitialState, OptClearValue, OptResourceRecycler);
			}

			GPUTexture(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(InResource, OptResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				GPUResource::Init(InResource, OptResourceRecycler);
			}
		};
	}
}