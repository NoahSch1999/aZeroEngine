#pragma once
#include "graphics_api/D3D12Include.hpp"
#include <algorithm>

namespace aZero
{
	namespace Helper
	{
		// TODO: Compute them once and then be done
		std::string GetProjectDirectory();
#ifdef _DEBUG
		std::string GetDebugProjectDirectory();
#endif
	}
}