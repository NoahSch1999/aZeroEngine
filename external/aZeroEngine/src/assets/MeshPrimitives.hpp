#pragma once
#include "render_api/D3D12Include.hpp"
#include <array>

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

	static inline constexpr uint32_t g_VerticesPerMeshlet = 64;
	static inline constexpr uint32_t g_PrimitivesPerMeshlet = 84;

	struct Meshlet
	{
		uint32_t VertexOffset;
		uint32_t VertexCount;
		uint32_t PrimitiveCount;
		std::array<uint32_t, g_PrimitivesPerMeshlet> Primitives;
	};

	void Meshletize( // Named it myself :))
		std::vector<DXM::Vector3>& positions,
		std::vector<aZero::Asset::Vertex>& vertices,
		std::vector<aZero::Asset::Index>& indices,
		std::vector<aZero::Asset::Meshlet>& outMeshlets,
		std::vector<DirectX::BoundingSphere>& outMeshletBounds
	);
}