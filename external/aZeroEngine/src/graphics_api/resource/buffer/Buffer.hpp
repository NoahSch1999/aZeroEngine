#pragma once
#include "graphics_api/resource/ResourceBase.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class Buffer : public ResourceBase
		{
		public:
			struct Desc
			{
				uint64_t NumBytes;
				D3D12_HEAP_TYPE AccessType;
				bool AllowUAV = false;
				D3D12_RESOURCE_STATES StartState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
				Desc() = default;
				Desc(uint64_t numBytes, D3D12_HEAP_TYPE accessType, bool allowUAV = false, D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
					:NumBytes(numBytes), AccessType(accessType), AllowUAV(allowUAV), StartState(startState) { }
			};

			Buffer() = default;
			Buffer(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{});
			Buffer(Buffer&& other) noexcept;
			Buffer& operator=(Buffer&& other) noexcept;

			bool IsDescValid(const Desc& desc);
			void* GetCPUAccessibleMemory() const { return m_MappedPtr; }

			void Write(const void* data, size_t size, size_t offset)
			{
				memcpy((char*)m_MappedPtr + offset, data, size);
			}
		private:
			void* m_MappedPtr = nullptr; // Will point to the cpu accessible memory unless the D3D12_HEAP_TYPE is D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, in which case it will be nullptr.
		};
	}
}