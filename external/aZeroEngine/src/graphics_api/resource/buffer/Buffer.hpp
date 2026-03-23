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
				Desc() = default;
				Desc(uint64_t numBytes, D3D12_HEAP_TYPE accessType)
					:NumBytes(numBytes), AccessType(accessType) {
				}
			};

			Buffer() = default;
			Buffer(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{});
			Buffer(Buffer&& other) noexcept;
			Buffer& operator=(Buffer&& other) noexcept;

			bool IsDescValid(const Desc& desc);
			void* GetCPUAccessibleMemory() const { return m_MappedPtr; }
		private:
			void* m_MappedPtr = nullptr; // Will point to the cpu accessible memory unless the D3D12_HEAP_TYPE is D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, in which case it will be nullptr.
		};
	}
}