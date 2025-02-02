#pragma once
#include <vector>
#include <mutex>

#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"
#include "misc/NonMovable.hpp"

namespace aZero
{
	namespace D3D12
	{
		class ResourceRecycler : public NonCopyable, public NonMovable
		{
		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_Resources;
			std::mutex m_Mutex;

		public:
			ResourceRecycler() = default;

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