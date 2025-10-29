#pragma once
#include "graphics_api/D3D12Include.hpp"
#include <algorithm>

namespace aZero
{
	namespace Helper
	{
		// TODO: These functions aren't nice. Re-think if and how they are needed and refactor...
		std::string GetProjectDirectory();
#if USE_DEBUG
		std::string GetDebugProjectDirectory();
#endif
	}
}