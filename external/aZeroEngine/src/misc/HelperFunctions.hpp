#pragma once
#include "graphics_api/D3D12Include.hpp"
#include <algorithm>

namespace aZero
{
	namespace Helper
	{
		// todo These functions aren't nice. Re-think if and how they are needed and refactor...
		std::string GetProjectDirectory();
#if USE_DEBUG
		std::string GetDebugProjectDirectory();
#endif
		//

		template<typename ValueType>
		std::string HandleNameCollision(const std::string& name, const std::unordered_map<std::string, ValueType>& map)
		{
			std::string tempName;
			while (map.count(name) > 0)
			{
				uint32_t incr = 0;
				tempName = name + "_" + std::to_string(incr);
				incr++;
			}

			if (tempName.length() > 0)
			{
				return tempName;
			}

			return name;
		}

		inline uint32_t Pack16To32(uint16_t low, uint16_t high) {
			return (static_cast<uint32_t>(high) << 16) | low;
		}
	}
}