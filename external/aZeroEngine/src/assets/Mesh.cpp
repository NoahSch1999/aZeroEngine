#include "Mesh.hpp"
#include "misc/EngineDebugMacros.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "meshoptimizer.h"
#include "misc/HelperFunctions.hpp"

aZero::Asset::MeshletMeshData GenerateMeshletData(
	const std::string& name, 
	const std::vector<aZero::Asset::Vertex>& vertices,
	std::vector<aZero::Asset::VertexIndex>& indices,
	const DirectX::BoundingSphere& bounds
)
{
	std::vector<aZero::Asset::Meshlet> finalMeshlets;
	std::vector<aZero::Asset::Vertex> finalVertices;

	const size_t max_vertices = 63;
	const size_t max_triangles = 21;

	const size_t max_meshlets = meshopt_buildMeshletsBound(
		indices.size(), max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> tempMeshlets;
	std::vector<aZero::Asset::VertexIndex> local_indices;
	std::vector<uint8_t> primitives;
	tempMeshlets.resize(max_meshlets);
	local_indices.resize(max_meshlets * max_vertices);
	primitives.resize(max_meshlets * max_triangles * 3);
	size_t meshlet_count = meshopt_buildMeshlets(tempMeshlets.data(), local_indices.data(), primitives.data(), indices.data(), indices.size(), &vertices[0].Position.x, vertices.size(), sizeof(aZero::Asset::Vertex), max_vertices, max_triangles, 0.f);

	const meshopt_Meshlet& last = tempMeshlets[meshlet_count - 1];

	/*local_indices.resize(last.vertex_offset + last.vertex_count);
	primitives.resize(last.triangle_offset + last.triangle_count * 3);
	tempMeshlets.resize(meshlet_count);*/

	for (const meshopt_Meshlet& meshlet : tempMeshlets)
	{
		uint32_t triangleOffset = meshlet.triangle_offset;
		uint32_t vertexOffset = meshlet.vertex_offset;

		for(int i = 0; i < meshlet.triangle_count * 3; i+=3)
		{
			uint8_t a = primitives[meshlet.triangle_offset + i];
			uint8_t b = primitives[meshlet.triangle_offset + i + 1];
			uint8_t c = primitives[meshlet.triangle_offset + i + 2];
			aZero::Asset::VertexIndex a2 = local_indices[vertexOffset + a];
			aZero::Asset::VertexIndex b2 = local_indices[vertexOffset + b];
			aZero::Asset::VertexIndex c2 = local_indices[vertexOffset + c];

			finalVertices.emplace_back(vertices[a2]);
			finalVertices.emplace_back(vertices[b2]);
			finalVertices.emplace_back(vertices[c2]);
		}

		aZero::Asset::Meshlet newMeshlet;
		newMeshlet.VertexOffset = vertexOffset;
		newMeshlet.TriangleCount = meshlet.triangle_count;

		meshopt_Bounds bounds = meshopt_computeMeshletBounds(&local_indices[meshlet.vertex_offset], &primitives[meshlet.triangle_offset],
			meshlet.triangle_count, &vertices[0].Position.x, vertices.size(), sizeof(aZero::Asset::Vertex));
		newMeshlet.Bounds = DirectX::BoundingSphere(DXM::Vector3(bounds.center[0], bounds.center[1], bounds.center[2]), bounds.radius);

		finalMeshlets.emplace_back(newMeshlet);
	}

	return {
		.Name = name,
		.Meshlets = std::move(finalMeshlets),
		.Vertices = std::move(finalVertices),
		.Bounds = bounds
	};
}

DirectX::BoundingSphere ComputeBoundingSphere(const std::vector<aZero::Asset::Vertex>& points)
{
	aZero::Asset::Vertex p0 = points[0];

	int i1 = 0;
	float maxDist = 0.0f;

	for (int i = 0; i < points.size(); i++)
	{
		float d = (points[i].Position - p0.Position).LengthSquared();
		if (d > maxDist)
		{
			maxDist = d;
			i1 = i;
		}
	}

	// 3. Find farthest point from i1
	int i2 = i1;
	maxDist = 0.0f;

	for (int i = 0; i < points.size(); i++)
	{
		float d = (points[i].Position - points[i1].Position).LengthSquared();
		if (d > maxDist)
		{
			maxDist = d;
			i2 = i;
		}
	}

	// 4. Initial sphere
	DXM::Vector3 center = (points[i1].Position + points[i2].Position) * 0.5f;
	float radius = (points[i2].Position - center).Length();

	// 5. Expand sphere
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

	return { DXM::Vector3(center.x, center.y, center.z), radius};
}

std::vector<aZero::Asset::MeshletMeshData> LoadFBX(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* const scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	std::vector<aZero::Asset::MeshletMeshData> output;
	if (!scene)
	{
		DEBUG_PRINT("Failed to load fbx at path: " + path);
		return output;
	}

	if (scene->mNumMeshes > 0)
	{
		output.reserve(scene->mNumMeshes);

		for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
		{
			const aiMesh* const mesh = scene->mMeshes[meshIndex];

			std::vector<aZero::Asset::Vertex> vertices;
			std::vector<aZero::Asset::VertexIndex> indices;

			vertices.reserve(mesh->mNumVertices);

			for (int i = 0; i < mesh->mNumVertices; i++)
			{
				aZero::Asset::Vertex tempVertex;
				tempVertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

				if (mesh->HasTextureCoords(0))
				{
					tempVertex.UV = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
				}

				if (mesh->HasNormals())
				{
					tempVertex.Normal = { mesh->mNormals[i].x,  mesh->mNormals[i].y,  mesh->mNormals[i].y };
					tempVertex.Normal.Normalize();
				}

				if (mesh->HasTangentsAndBitangents())
				{
					tempVertex.Tangent = { mesh->mTangents[i].x,  mesh->mTangents[i].y,  mesh->mTangents[i].y };
					tempVertex.Tangent.Normalize();
				}

				vertices.emplace_back(tempVertex);
			}

			indices.reserve(mesh->mNumFaces * 3);
			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				const aiFace& face = mesh->mFaces[i];

				indices.emplace_back(face.mIndices[0]);
				indices.emplace_back(face.mIndices[1]);
				indices.emplace_back(face.mIndices[2]);
			}

			output.emplace_back(GenerateMeshletData(mesh->mName.C_Str(), vertices, indices, ComputeBoundingSphere(vertices)));
		}
	}
	return output;
}

std::vector<aZero::Asset::MeshletMeshData> aZero::Asset::LoadFromFile(const std::string& filePath)
{
	const std::string suffix = Helper::GetPathSuffix(filePath);
	if (suffix == "fbx")
	{
		return LoadFBX(filePath);
	}

	// Add other formats here...

	return std::vector<aZero::Asset::MeshletMeshData>();
}

bool aZero::Asset::Mesh::LoadFromFile(const std::string& filePath)
{
	const auto& meshes = Asset::LoadFromFile(filePath);
	if (meshes.size())
	{
		m_VertexData = meshes[0];
		AssetBase::Load(filePath);
	}

	return !meshes.empty();
}