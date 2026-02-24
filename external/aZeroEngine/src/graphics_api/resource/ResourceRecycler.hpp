#pragma once
#include <vector>
#include <mutex>

#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class ResourceRecycler
		{
		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_Resources;
			std::mutex m_Lock;
		public:
			ResourceRecycler() = default;
			void Append(Microsoft::WRL::ComPtr<ID3D12Resource>&& resource) {
				std::unique_lock<std::mutex> lock(m_Lock);
				m_Resources.emplace_back(std::move(resource));
			}
			void Clear() {
				std::unique_lock<std::mutex> lock(m_Lock);
				m_Resources.clear();
			}
		};
	}
}