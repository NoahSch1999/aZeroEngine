#pragma once
#include <optional>

#include "misc/NonCopyable.hpp"
#include "renderer/ResourceRecycler.hpp"

namespace aZero
{
	namespace D3D12
	{
		// TODO: Rework at some point, or move to enhanced barriers
		struct ResourceTransitionBundles
		{
			D3D12_RESOURCE_STATES StateBefore, StateAfter;
			ID3D12Resource* ResourcePtr;
		};

		void TransitionResources(ID3D12GraphicsCommandList* CmdList, const std::vector<ResourceTransitionBundles>& Bundles);
		//

		class GPUResource : public NonCopyable
		{
		protected:
			D3D12::ResourceRecycler* m_ResourceRecycler = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
			void* m_MappedPtr = nullptr;

			void Map(D3D12_HEAP_TYPE Type)
			{
				if (Type == D3D12_HEAP_TYPE_UPLOAD || Type == D3D12_HEAP_TYPE_GPU_UPLOAD || Type == D3D12_HEAP_TYPE_READBACK)
				{
					m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedPtr));
				}
			}

			GPUResource(
				ID3D12Device* Device,
				D3D12_HEAP_TYPE HeapType,
				const D3D12_RESOURCE_DESC& Desc,
				D3D12::ResourceRecycler& ResourceRecycler,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{}
			)
			{
				this->Init(Device, HeapType, Desc, ResourceRecycler, InitialState, OptClearValue);
			}

			void Init(
				ID3D12Device* Device,
				D3D12_HEAP_TYPE HeapType,
				const D3D12_RESOURCE_DESC& Desc,
				D3D12::ResourceRecycler& ResourceRecycler,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{}
			)
			{
				this->Reset();

				D3D12_HEAP_PROPERTIES HeapProperties;
				ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
				HeapProperties.Type = HeapType;

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

				this->Map(HeapType);

				m_ResourceRecycler = &ResourceRecycler;
			}

			GPUResource(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Init(InResource, ResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Reset();

				m_Resource = InResource;
				m_ResourceRecycler = &ResourceRecycler;

				D3D12_HEAP_PROPERTIES HeapProperties;
				D3D12_HEAP_FLAGS HeapFlags;
				const HRESULT Res = InResource->GetHeapProperties(&HeapProperties, &HeapFlags);
				if (FAILED(Res))
				{
					throw std::invalid_argument("GPUResource::Init() => Failed to create commited resource");
				}

				this->Map(HeapProperties.Type);
			}

		public:
			GPUResource() = default;

			~GPUResource()
			{
				this->Reset();
			}

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


			void Reset()
			{
				if (m_ResourceRecycler && m_Resource)
				{
					m_ResourceRecycler->RecycleResource(m_Resource);
					m_Resource = nullptr;
					m_MappedPtr = nullptr;
					m_ResourceRecycler = nullptr;
				}
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
				D3D12_HEAP_TYPE HeapType,
				uint64_t NumBytes,
				D3D12::ResourceRecycler& ResourceRecycler
			)
			{
				this->Init(Device, HeapType, NumBytes, ResourceRecycler);
			}

			void Init(
				ID3D12Device* Device,
				D3D12_HEAP_TYPE HeapType,
				uint64_t NumBytes,
				D3D12::ResourceRecycler& ResourceRecycler
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

				GPUResource::Init(Device, HeapType, Desc, ResourceRecycler, D3D12_RESOURCE_STATE_COMMON, std::optional<D3D12_CLEAR_VALUE>{});
			}

			GPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Init(InResource, ResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, D3D12::ResourceRecycler& ResourceRecycler)
			{
				GPUResource::Init(InResource, ResourceRecycler);
			}

			void Resize(ID3D12GraphicsCommandList* CmdList, uint32_t NewSizeBytes)
			{
				D3D12_RESOURCE_DESC Desc = m_Resource->GetDesc();
				const uint32_t OldSizeBytes = Desc.Width;

				Desc.Width = NewSizeBytes;

				D3D12_HEAP_PROPERTIES Props;
				HRESULT Res = m_Resource->GetHeapProperties(&Props, NULL);
				if (FAILED(Res))
				{
					throw std::invalid_argument("NA");
				}

				ID3D12Device* Device;
				Res = m_Resource->GetDevice(IID_PPV_ARGS(&Device));
				if (FAILED(Res))
				{
					throw std::invalid_argument("NA");
				}

				Microsoft::WRL::ComPtr<ID3D12Resource> NewResource;
				Res = Device->CreateCommittedResource(
					&Props, D3D12_HEAP_FLAG_NONE, &Desc,
					D3D12_RESOURCE_STATE_COMMON, nullptr,
					IID_PPV_ARGS(NewResource.GetAddressOf()));

				if (FAILED(Res))
				{
					throw std::invalid_argument("NA");
				}

				CmdList->CopyBufferRegion(NewResource.Get(), 0, m_Resource.Get(), 0, OldSizeBytes);

				if (this->m_ResourceRecycler)
				{
					this->GetResourceRecycler()->RecycleResource(m_Resource);
				}

				m_Resource = NewResource;

				this->Map(Props.Type);
			}

			void Write(ID3D12GraphicsCommandList* CmdList, void* Data, uint32_t NumBytes, uint32_t ByteOffset = 0)
			{
				const uint32_t MemorySizeBytes = static_cast<uint32_t>(m_Resource->GetDesc().Width);
				const uint32_t LastOffsetBytes = ByteOffset + NumBytes;
				if (!this->GetCPUAccessibleMemory() || (MemorySizeBytes < LastOffsetBytes))
				{
					throw std::out_of_range("GPUBuffer::Write() => OOR write to resource");
				}

				memcpy((char*)this->GetCPUAccessibleMemory() + ByteOffset, Data, NumBytes);
			}
		};

		class GPUTexture : public GPUResource
		{
		private:
			std::optional<D3D12_CLEAR_VALUE> m_ClearValue;

			void MoveOp(GPUTexture&& Other)
			{
				m_ClearValue = Other.m_ClearValue;
				Other.m_ClearValue.reset();
				GPUResource::operator=(std::move(Other));
			}

		public:
			GPUTexture() = default;

			GPUTexture(GPUTexture&& Other) noexcept
			{
				this->MoveOp(std::forward<GPUTexture>(Other));
			}

			GPUTexture& operator=(GPUTexture&& Other) noexcept
			{
				if (this != &Other)
				{
					this->MoveOp(std::forward<GPUTexture>(Other));
				}
				return *this;
			}

			GPUTexture(
				ID3D12Device* Device,
				const DXM::Vector3& Dimensions,
				DXGI_FORMAT Format,
				D3D12_RESOURCE_FLAGS Flags,
				D3D12::ResourceRecycler& ResourceRecycler,
				uint32_t MipLevels,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{}
			)
			{
				this->Init(Device, Dimensions, Format, Flags, ResourceRecycler, MipLevels, InitialState, OptClearValue);
			}

			void Init(
				ID3D12Device* Device,
				const DXM::Vector3& Dimensions,
				DXGI_FORMAT Format,
				D3D12_RESOURCE_FLAGS Flags,
				D3D12::ResourceRecycler& ResourceRecycler,
				uint32_t MipLevels,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{}
			)
			{
				this->Reset();

				m_ClearValue = OptClearValue;

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
				Desc.Width = static_cast<UINT64>(Dimensions.x);
				Desc.Height = static_cast<UINT>(Dimensions.y);
				Desc.DepthOrArraySize = static_cast<UINT16>(Dimensions.z);
				Desc.MipLevels = MipLevels;
				Desc.SampleDesc.Count = 1;
				Desc.SampleDesc.Quality = 0;
				Desc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
				Desc.Flags = Flags;

				GPUResource::Init(Device, D3D12_HEAP_TYPE_DEFAULT, Desc, ResourceRecycler, InitialState, m_ClearValue);
			}

			GPUTexture(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Init(InResource, ResourceRecycler);
			}

			void Init(Microsoft::WRL::ComPtr<ID3D12Resource> InResource, D3D12::ResourceRecycler& ResourceRecycler)
			{
				this->Reset();
				GPUResource::Init(InResource, ResourceRecycler);
			}

			void Reset()
			{
				m_ClearValue.reset();
				GPUResource::Reset();
			}

			void Resize(const DXM::Vector3& Dimensions,
				D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON,
				std::optional<D3D12_CLEAR_VALUE> OptClearValue = std::optional<D3D12_CLEAR_VALUE>{})
			{
				ID3D12Device* Device;
				const HRESULT GetDeviceRes = this->GetResource()->GetDevice(IID_PPV_ARGS(&Device));
				if (FAILED(GetDeviceRes))
				{
					throw std::invalid_argument("Failed to get device when resizing texture");
				}
				auto Desc = this->GetResource()->GetDesc();
				Desc.Width = static_cast<UINT64>(Dimensions.x);
				Desc.Height = static_cast<UINT>(Dimensions.y);
				Desc.DepthOrArraySize = static_cast<UINT16>(Dimensions.z);
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
				GPUResource::Init(Device, D3D12_HEAP_TYPE_DEFAULT, Desc, *this->GetResourceRecycler(), InitialState, OptClearValue);
			}

			std::optional<D3D12_CLEAR_VALUE> GetClearValue() const { return m_ClearValue; }
		};
	}
}