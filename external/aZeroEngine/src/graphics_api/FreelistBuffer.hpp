#pragma once
#include "graphics_api/Wrappers/ResourceWrapping.hpp"
#include "misc/IndexFreelist.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		template<typename T>
		class NewFreelistBuffer
		{
		private:
			DS::IndexFreelist<uint32_t> m_Freelist;
			RenderAPI::Buffer m_Buffer;
		public:
			NewFreelistBuffer() = default;
			NewFreelistBuffer(ID3D12Device* device, uint32_t maxElements, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{})
			{
				RenderAPI::Buffer::Desc desc(sizeof(T) * maxElements, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);
				m_Buffer = RenderAPI::Buffer(device, desc, opt_diResourceRecycler);
			}

			uint32_t Allocate()
			{
				return m_Freelist.New();
			}

			void Deallocate(uint32_t index)
			{
				m_Freelist.Recycle(index);
			}

			uint32_t GetByteOffset(uint32_t index) const
			{
				return index * sizeof(T);
			}

			RenderAPI::Buffer& GetBuffer() { return m_Buffer; }
		};
	}
}