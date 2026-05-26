#include "Mesh.hpp"
#include "misc/EngineDebugMacros.hpp"
#include "FBX_Loading.hpp"
#include "meshoptimizer.h"
#include "misc/HelperFunctions.hpp"

void Meshletize( // Named it myself :))
	std::vector<aZero::Asset::Vertex>& vertices,
	std::vector<aZero::Asset::Index>& indices,
	std::vector<aZero::Asset::Meshlet>& outMeshlets,
	std::vector<uint32_t>& outPrimitives,
	uint32_t primitiveOffset, uint32_t vertexOffset
)
{
	const size_t max_vertices = 64;
	const size_t max_triangles = 126;

	const size_t max_meshlets = meshopt_buildMeshletsBound(
		indices.size(), max_vertices, max_triangles);

	meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
	meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &vertices[0].Position.x, vertices.size(), sizeof(vertices[0]), 1.05f);

	std::vector<unsigned int> remap(vertices.size());

	meshopt_optimizeVertexFetchRemap(
		remap.data(),
		indices.data(),
		indices.size(),
		vertices.size()
	);

	meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertices.size(), sizeof(vertices[0]), remap.data());

	meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());

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
	for (int i = 0; i < meshlet_count; i++)
	{
		const meshopt_Meshlet& meshlet = tempMeshlets[i];
		aZero::Asset::Meshlet newMeshlet;

		meshopt_Bounds bounds = meshopt_computeMeshletBounds(&local_indices[meshlet.vertex_offset], &primitives[meshlet.triangle_offset],
			meshlet.triangle_count, &vertices[0].Position.x, vertices.size(), sizeof(vertices[0]));
		newMeshlet.Bounds = DirectX::BoundingSphere(DXM::Vector3(bounds.center[0], bounds.center[1], bounds.center[2]), bounds.radius);

		newMeshlet.PrimitiveCount = meshlet.triangle_count;
		newMeshlet.VertexCount = meshlet.vertex_count;
		newMeshlet.VertexOffset = meshlet.vertex_offset + vertexOffset;
		newMeshlet.PrimitiveOffset = meshlet.triangle_offset / 3 + primitiveOffset;

		for (int j = 0; j < meshlet.triangle_count; j++)
		{
			const uint32_t primOffset = meshlet.triangle_offset + j * 3;
			outPrimitives.emplace_back(aZero::Helper::Pack8To32(primitives[primOffset], primitives[primOffset + 1], primitives[primOffset + 2], 0));
		}

		outMeshlets.emplace_back(newMeshlet);
	}

	indices = std::move(local_indices);
}

aZero::Asset::Mesh::Mesh(const FBX::FBX_Mesh& mesh)
{
	for (const FBX::FBX_Submesh& submesh : mesh.Submeshes)
	{
		std::vector<Vertex> vertices(submesh.Vertices);
		std::vector<Index> indices(submesh.Indices);
		std::vector<Meshlet> meshlets;
		std::vector<uint32_t> primitives;
		Meshletize(vertices, indices, meshlets, primitives, m_VertexData.Primitives.size(), m_VertexData.Indices.size());

		for (auto& index : indices)
		{
			index += m_VertexData.Vertices.size();
		}
		
		Submesh newSubmesh;
		newSubmesh.Name = submesh.Name;
		newSubmesh.Bounds = submesh.Bounds;
		newSubmesh.MeshletOffset = m_VertexData.Meshlets.size();
		newSubmesh.MeshletCount = meshlets.size();

		m_VertexData.Vertices.insert(m_VertexData.Vertices.end(), vertices.begin(), vertices.end());
		m_VertexData.Indices.insert(m_VertexData.Indices.end(), indices.begin(), indices.end());
		m_VertexData.Meshlets.insert(m_VertexData.Meshlets.end(), meshlets.begin(), meshlets.end());
		m_VertexData.Primitives.insert(m_VertexData.Primitives.end(), primitives.begin(), primitives.end());

		m_Submeshes.push_back(newSubmesh);
	}
}