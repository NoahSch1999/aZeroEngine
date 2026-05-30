#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero::Asset
{
	using Index = uint32_t;

	struct Vertex
	{
		uint16_t UV[2];
		uint16_t Normal[2];
	};

	inline uint16_t FloatToUNorm16(float v)
	{
		v = std::fmin(std::fmax(v, 0.0f), 1.0f);
		return (uint16_t)(v * 65535.0f + 0.5f);
	}

	inline Vertex PackVertex(const DXM::Vector2& encNormal, const DXM::Vector2& uv)
	{
		Vertex p{};

		p.Normal[0] = FloatToUNorm16(encNormal.x * 0.5f + 0.5f);
		p.Normal[1] = FloatToUNorm16(encNormal.y * 0.5f + 0.5f);

		p.UV[0] = FloatToUNorm16(uv.x);
		p.UV[1] = FloatToUNorm16(uv.y);

		return p;
	}
}