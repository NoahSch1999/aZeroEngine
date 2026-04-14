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
			uint32_t m_FrameIndex = 0;
			std::vector<std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>> m_Resources;
		public:
			ResourceRecycler() = default;
			ResourceRecycler(uint32_t frameInFlight)
			{
				m_Resources.resize(frameInFlight);
			}

			void SetFrameIndex(uint32_t frameIndex) { m_FrameIndex = frameIndex; }

			void Append(Microsoft::WRL::ComPtr<ID3D12Resource>&& resource) {
				m_Resources[m_FrameIndex].emplace_back(std::move(resource));
			}

			void Clear() {
				m_Resources[m_FrameIndex - m_Resources.size() % m_Resources.size()].clear();
			}
		};
	}
}