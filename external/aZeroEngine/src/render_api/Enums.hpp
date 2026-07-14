#pragma once
#include <ktx.h>
#include "render_api/D3D12Include.hpp"

namespace aZero::RenderAPI
{
	enum class TEXTURE_FORMAT { RGBA8_UNORM, RGBA8_UNORM_SRGB, BC7_UNORM, BC7_UNORM_SRGB, UNKNOWN };

	static inline TEXTURE_FORMAT FromVK_Format(ktx_uint32_t format)
	{
		switch (format)
		{
			// VK_FORMAT_R8G8B8A8_UNORM
		case 37:
			return TEXTURE_FORMAT::RGBA8_UNORM;
			// VK_FORMAT_R8G8B8A8_SRGB 
		case 43:
			return TEXTURE_FORMAT::RGBA8_UNORM;
		case 145:
			return TEXTURE_FORMAT::BC7_UNORM;
		case 146:
			return TEXTURE_FORMAT::BC7_UNORM_SRGB;
		default:
			return TEXTURE_FORMAT::UNKNOWN;
		}
	}

	static inline TEXTURE_FORMAT FromDX_Format(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			return TEXTURE_FORMAT::RGBA8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return TEXTURE_FORMAT::RGBA8_UNORM_SRGB;

		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM:
			return TEXTURE_FORMAT::BC7_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return TEXTURE_FORMAT::BC7_UNORM_SRGB;

		default:
			return TEXTURE_FORMAT::UNKNOWN;
		}
	}

	static inline DXGI_FORMAT ToDX_Format(TEXTURE_FORMAT format)
	{
		switch (format)
		{
		case TEXTURE_FORMAT::RGBA8_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

		case TEXTURE_FORMAT::RGBA8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		case TEXTURE_FORMAT::BC7_UNORM:
			return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM;

		case TEXTURE_FORMAT::BC7_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB;

		default:
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		}
	}
}