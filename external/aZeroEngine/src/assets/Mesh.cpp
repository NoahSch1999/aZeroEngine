#include "Mesh.hpp"
#include "misc/EngineDebugMacros.hpp"
#include "FBX_Loading.hpp"
#include "meshoptimizer.h"
#include "misc/HelperFunctions.hpp"

void Meshletize( // Named it myself :))
	std::vector<DXM::Vector3>& positions,
	std::vector<aZero::Asset::Vertex>& vertices,
	std::vector<aZero::Asset::Index>& indices,
	std::vector<aZero::Asset::Meshlet>& outMeshlets,
	std::vector<DirectX::BoundingSphere>& outMeshletBounds
)
{
	const size_t max_vertices = aZero::Asset::g_VerticesPerMeshlet;
	const size_t max_triangles = aZero::Asset::g_PrimitivesPerMeshlet;

	const size_t max_meshlets = meshopt_buildMeshletsBound(
		indices.size(), max_vertices, max_triangles);

	meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), positions.size());
	meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &positions[0].x, positions.size(), sizeof(positions[0]), 1.05f);

	std::vector<unsigned int> remap(vertices.size());

	meshopt_optimizeVertexFetchRemap(
		remap.data(),
		indices.data(),
		indices.size(),
		positions.size()
	);

	meshopt_remapVertexBuffer(positions.data(), positions.data(), positions.size(), sizeof(positions[0]), remap.data());
	meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertices.size(), sizeof(vertices[0]), remap.data());

	meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());

	meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), positions.size());

	std::vector<meshopt_Meshlet> tempMeshlets;
	std::vector<aZero::Asset::Index> local_indices;
	std::vector<uint8_t> primitives;
	tempMeshlets.resize(max_meshlets);
	local_indices.resize(max_meshlets * max_vertices);
	primitives.resize(max_meshlets * max_triangles * 3);
	size_t meshlet_count = meshopt_buildMeshlets(tempMeshlets.data(), local_indices.data(), primitives.data(), indices.data(),
		indices.size(), &positions[0].x, positions.size(), sizeof(positions[0]), max_vertices, max_triangles, 0.f);

	const meshopt_Meshlet& last = tempMeshlets[meshlet_count - 1];

	local_indices.resize(last.vertex_offset + last.vertex_count);
	primitives.resize(last.triangle_offset + last.triangle_count * 3);
	tempMeshlets.resize(meshlet_count);

	outMeshlets.reserve(meshlet_count);

	std::vector<DXM::Vector3> outPositions;
	std::vector<aZero::Asset::Vertex> outVertices;
	for (uint32_t i = 0; i < meshlet_count; i++)
	{
		const meshopt_Meshlet& meshlet = tempMeshlets[i];
		aZero::Asset::Meshlet newMeshlet;

		meshopt_Bounds bounds = meshopt_computeMeshletBounds(&local_indices[meshlet.vertex_offset], &primitives[meshlet.triangle_offset],
			meshlet.triangle_count, &positions[0].x, positions.size(), sizeof(positions[0]));
		outMeshletBounds.emplace_back(DXM::Vector3(bounds.center[0], bounds.center[1], bounds.center[2]), bounds.radius);

		newMeshlet.PrimitiveCount = meshlet.triangle_count;
		newMeshlet.VertexCount = meshlet.vertex_count;
		newMeshlet.VertexOffset = meshlet.vertex_offset;

		for (uint32_t h = 0; h < meshlet.vertex_count; h++)
		{
			outPositions.push_back(positions[local_indices[meshlet.vertex_offset + h]]);
			outVertices.push_back(vertices[local_indices[meshlet.vertex_offset + h]]);
		}

		for (uint32_t j = 0; j < meshlet.triangle_count; j++)
		{
			const uint32_t primOffset = meshlet.triangle_offset + j * 3;
			newMeshlet.Primitives[j] = aZero::Helper::Pack8To32(primitives[primOffset], primitives[primOffset + 1], primitives[primOffset + 2], 0);
		}

		outMeshlets.emplace_back(newMeshlet);
	}

	positions = outPositions;
	vertices = outVertices;
}

aZero::Asset::Mesh::Mesh(const FBX::FBX_Mesh& mesh)
{
	for (const FBX::FBX_Submesh& submesh : mesh.Submeshes)
	{
		std::vector<Vertex> vertices(submesh.Vertices);
		std::vector<DXM::Vector3> positions(submesh.Positions);
		std::vector<Index> indices(submesh.Indices);
		std::vector<Meshlet> meshlets;
		std::vector<DirectX::BoundingSphere> meshletBounds;
		Meshletize(positions, vertices, indices, meshlets, meshletBounds);
		
		Submesh newSubmesh;
		newSubmesh.Name = submesh.Name;
		newSubmesh.Bounds = submesh.Bounds;
		newSubmesh.MeshletOffset = m_VertexData.Meshlets.size();
		newSubmesh.MeshletCount = meshlets.size();

		m_VertexData.Positions.insert(m_VertexData.Positions.end(), positions.begin(), positions.end());
		m_VertexData.Vertices.insert(m_VertexData.Vertices.end(), vertices.begin(), vertices.end());
		m_VertexData.Meshlets.insert(m_VertexData.Meshlets.end(), meshlets.begin(), meshlets.end());
		m_VertexData.MeshletBounds.insert(m_VertexData.MeshletBounds.end(), meshletBounds.begin(), meshletBounds.end());

		m_Submeshes.push_back(newSubmesh);
	}
}