#pragma once
#include <vector>

#include "Core/D3D12Core.h"

namespace aZero
{
	namespace D3D12
	{
		// TODO - Make it threadsafe.
		// TODO - Make it more efficient by minimizing the amount of dynamic memory allocations
		class ResourceRecycler
		{
		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_resources;

		public:
			ResourceRecycler()
			{

			}

			~ResourceRecycler()
			{

			}

			void RecycleResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
			{
				m_resources.emplace_back(resource);
			}

			void Clear()
			{
				m_resources.clear();
			}
		};
	}

	extern D3D12::ResourceRecycler gResourceRecycler;
}