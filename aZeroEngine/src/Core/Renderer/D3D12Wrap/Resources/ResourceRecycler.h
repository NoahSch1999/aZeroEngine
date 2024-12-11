#pragma once
#include <vector>
#include <mutex>

#include "Core/D3D12Include.h"

namespace aZero
{
	namespace D3D12
	{
		// TODO - Make it more efficient by minimizing the amount of dynamic memory allocations
		class ResourceRecycler
		{
		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_Resources;
			std::mutex m_Mutex;

		public:
			ResourceRecycler() = default;

			ResourceRecycler(const ResourceRecycler&) = delete;
			ResourceRecycler& operator=(const ResourceRecycler&) = delete;
			ResourceRecycler(ResourceRecycler&&) = delete;
			ResourceRecycler& operator=(ResourceRecycler&&) = delete;

			void RecycleResource(Microsoft::WRL::ComPtr<ID3D12Resource> Resource)
			{
				std::unique_lock<std::mutex>(m_Mutex);
				m_Resources.emplace_back(Resource);
			}

			void ReleaseResources()
			{
				std::unique_lock<std::mutex>(m_Mutex);
				m_Resources.clear();
			}
		};
	}
}