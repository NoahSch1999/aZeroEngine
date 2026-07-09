#pragma once
#include "render_api/D3D12Include.hpp"
#include <meshoptimizer.h>
#include <array>
#include <vector>

namespace aZero::Asset
{
	using Index = uint32_t;

	struct Vertex
	{
		DXM::Vector4 Position;
		uint16_t UV[2];
		uint16_t Normal[2];
	};

	inline uint16_t FloatToUNorm16(float v)
	{
		v = std::clamp(v, 0.0f, 1.0f);
		return (uint16_t)(v * 65535.0f + 0.5f);
	}

	inline std::array<uint16_t, 2> PackNormal(const DXM::Vector2& normal)
	{
		return { FloatToUNorm16(normal.x * 0.5f + 0.5f), FloatToUNorm16(normal.y * 0.5f + 0.5f) };
	}

	inline std::array<uint16_t, 2> PackUV(const DXM::Vector2& uv)
	{
		return { DirectX::PackedVector::XMConvertFloatToHalf(uv.x), DirectX::PackedVector::XMConvertFloatToHalf(uv.y) };
	}

	inline Vertex PackVertex(const DXM::Vector2& normal, const DXM::Vector2& uv)
	{
		Vertex p{ };

		auto n = PackNormal(normal);
		auto uvOut = PackUV(uv);

		std::copy(n.begin(), n.end(), p.Normal);
		std::copy(uvOut.begin(), uvOut.end(), p.UV);

		return p;
	}

	inline DirectX::BoundingSphere ComputeBoundingSphere(const std::vector<Asset::Vertex>& points)
	{
		DXM::Vector3 p0 = { points[0].Position.x, points[0].Position.y, points[0].Position.z };

		int i1 = 0;
		float maxDist = 0.0f;

		for (int i = 0; i < points.size(); i++)
		{
			float d = (DXM::Vector3(points[i].Position.x, points[i].Position.y, points[i].Position.z) - p0).LengthSquared();
			if (d > maxDist)
			{
				maxDist = d;
				i1 = i;
			}
		}

		int i2 = i1;
		maxDist = 0.0f;

		for (int i = 0; i < points.size(); i++)
		{
			float d = (DXM::Vector3(points[i].Position.x, points[i].Position.y, points[i].Position.z) - DXM::Vector3(points[i1].Position.x, points[i1].Position.y, points[i1].Position.z)).LengthSquared();
			if (d > maxDist)
			{
				maxDist = d;
				i2 = i;
			}
		}

		DXM::Vector3 center = DXM::Vector3(points[i1].Position.x, points[i1].Position.y, points[i1].Position.z) + DXM::Vector3(points[i2].Position.x, points[i2].Position.y, points[i2].Position.z) * 0.5f;
		float radius = (DXM::Vector3(points[i2].Position.x, points[i2].Position.y, points[i2].Position.z) - center).Length();

		for (const auto& p : points)
		{
			DXM::Vector3 d = p.Position - center;
			float dist = d.Length();

			if (dist > radius)
			{
				float newRadius = (radius + dist) * 0.5f;
				float k = (newRadius - radius) / dist;

				center += d * k;
				radius = newRadius;
			}
		}

		return { DXM::Vector3(center.x, center.y, center.z), radius };
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

	inline void Meshletize( // Named it myself :))
		std::vector<aZero::Asset::Vertex>& vertices,
		std::vector<aZero::Asset::Index>& indices,
		std::vector<aZero::Asset::Meshlet>& outMeshlets,
		std::vector<DirectX::BoundingSphere>& outMeshletBounds,
		uint32_t vertexBaseOffset
	)
	{
		const size_t max_vertices = aZero::Asset::g_VerticesPerMeshlet;
		const size_t max_triangles = aZero::Asset::g_PrimitivesPerMeshlet;

		const size_t max_meshlets = meshopt_buildMeshletsBound(
			indices.size(), max_vertices, max_triangles);

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
		meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &vertices[0].Position.x, vertices.size(), sizeof(vertices[0]), 1.05f);

		meshopt_optimizeVertexFetch(
			vertices.data(),
			indices.data(),
			indices.size(),
			vertices.data(),
			vertices.size(),
			sizeof(vertices[0])
		);

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

		std::vector<meshopt_Meshlet> tempMeshlets;
		std::vector<aZero::Asset::Index> local_indices;
		std::vector<uint8_t> primitives;
		tempMeshlets.resize(max_meshlets);
		local_indices.resize(max_meshlets * max_vertices);
		primitives.resize(max_meshlets * max_triangles * 3);
		size_t meshlet_count = meshopt_buildMeshlets(tempMeshlets.data(), local_indices.data(), primitives.data(), indices.data(),
			indices.size(), &vertices[0].Position.x, vertices.size(), sizeof(vertices[0]), max_vertices, max_triangles, 0.f);

		const meshopt_Meshlet& last = tempMeshlets[meshlet_count - 1];

		local_indices.resize(last.vertex_offset + last.vertex_count);
		primitives.resize(last.triangle_offset + last.triangle_count * 3);
		tempMeshlets.resize(meshlet_count);

		outMeshlets.reserve(meshlet_count);

		std::vector<aZero::Asset::Vertex> outVertices;
		for (uint32_t i = 0; i < meshlet_count; i++)
		{
			const meshopt_Meshlet& meshlet = tempMeshlets[i];
			aZero::Asset::Meshlet newMeshlet;

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(&local_indices[meshlet.vertex_offset], &primitives[meshlet.triangle_offset],
				meshlet.triangle_count, &vertices[0].Position.x, vertices.size(), sizeof(vertices[0]));
			outMeshletBounds.emplace_back(DXM::Vector3(bounds.center[0], bounds.center[1], bounds.center[2]), bounds.radius);

			newMeshlet.PrimitiveCount = meshlet.triangle_count;
			newMeshlet.VertexCount = meshlet.vertex_count;

			for (uint32_t h = 0; h < meshlet.vertex_count; h++)
			{
				outVertices.push_back(vertices[local_indices[meshlet.vertex_offset + h]]);
			}

			for (uint32_t j = 0; j < meshlet.triangle_count; j++)
			{
				const uint32_t primOffset = meshlet.triangle_offset + j * 3;
				newMeshlet.Primitives[j] = aZero::Helper::Pack8To32(primitives[primOffset], primitives[primOffset + 1], primitives[primOffset + 2], 0);
			}

			newMeshlet.VertexOffset = meshlet.vertex_offset + vertexBaseOffset;
			outMeshlets.emplace_back(newMeshlet);
		}

		vertices = outVertices;
	}
}