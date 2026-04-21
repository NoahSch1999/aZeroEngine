#include "Mesh.hpp"
#include "misc/EngineDebugMacros.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "meshoptimizer.h"
#include "misc/HelperFunctions.hpp"

aZero::Asset::MeshletMeshData GenerateMeshletData(
	const std::string& name, 
	const std::vector<aZero::Asset::VertexPosition>& positions, 
	const std::vector<aZero::Asset::GenericVertexData>& genericVertexData, 
	std::vector<aZero::Asset::VertexIndex>& meshIndices,
	const DirectX::BoundingSphere& bounds
)
{
	const size_t max_vertices = 64;
	const size_t max_triangles = 126;

	const size_t max_meshlets = meshopt_buildMeshletsBound(
		positions.size(), max_vertices, max_triangles);

	// TODO: Take a look if there's any unneccessary ops
	meshopt_optimizeVertexCache(meshIndices.data(), meshIndices.data(), meshIndices.size(), positions.size());
	meshopt_optimizeOverdraw(meshIndices.data(), meshIndices.data(), meshIndices.size(), &positions[0].x, positions.size(), sizeof(aZero::Asset::VertexPosition), 1.05f);
	
	std::vector<unsigned int> remap(positions.size());

	meshopt_optimizeVertexFetchRemap(
		remap.data(),
		meshIndices.data(),
		meshIndices.size(),
		positions.size()
	);

	std::vector<aZero::Asset::VertexPosition> newPositions(positions.size());
	meshopt_remapVertexBuffer(newPositions.data(), positions.data(), positions.size(), sizeof(aZero::Asset::VertexPosition), remap.data());

	std::vector<aZero::Asset::GenericVertexData> newGeneric(genericVertexData.size());
	meshopt_remapVertexBuffer(newGeneric.data(), genericVertexData.data(), genericVertexData.size(), sizeof(aZero::Asset::GenericVertexData), remap.data());


	meshopt_remapIndexBuffer(meshIndices.data(), meshIndices.data(), meshIndices.size(), remap.data());

	std::vector<meshopt_Meshlet> tempMeshlets;
	std::vector<aZero::Asset::VertexIndex> local_indices;
	std::vector<uint8_t> primitives;
	tempMeshlets.resize(max_meshlets);
	local_indices.resize(max_meshlets * max_vertices);
	primitives.resize(max_meshlets * max_triangles * 3);
	size_t meshlet_count = meshopt_buildMeshlets(tempMeshlets.data(), local_indices.data(), primitives.data(), meshIndices.data(), meshIndices.size(), &newPositions[0].x, newPositions.size(), sizeof(aZero::Asset::VertexPosition), max_vertices, max_triangles, 0.f);

	const meshopt_Meshlet& last = tempMeshlets[meshlet_count - 1];

	local_indices.resize(last.vertex_offset + last.vertex_count);
	primitives.resize(last.triangle_offset + last.triangle_count * 3);
	tempMeshlets.resize(meshlet_count);

	std::vector<aZero::Asset::Meshlet> finalMeshlets;
	finalMeshlets.reserve(tempMeshlets.size());
	for (const meshopt_Meshlet& meshlet : tempMeshlets)
	{
		meshopt_optimizeMeshlet(&local_indices[meshlet.vertex_offset], &primitives[meshlet.triangle_offset], meshlet.triangle_count, meshlet.vertex_count);
		meshopt_Bounds bounds = meshopt_computeMeshletBounds(&local_indices[meshlet.vertex_offset], &primitives[meshlet.triangle_offset],
			meshlet.triangle_count, &newPositions[0].x, newPositions.size(), sizeof(aZero::Asset::VertexPosition));

		// Divide meshlet.triangle_offset with 3 since we pack the primitive indices in a 32bit uint
		finalMeshlets.emplace_back(meshlet.vertex_count, meshlet.vertex_offset, meshlet.triangle_count, meshlet.triangle_offset / 3, DirectX::BoundingSphere(DXM::Vector3(bounds.center[0], bounds.center[1], bounds.center[2]), bounds.radius));
	}

	std::vector<uint32_t> newPrims;
	newPrims.reserve(primitives.size() + primitives.size() / 3);
	for (int i = 0; i < primitives.size(); i += 3)
	{
		newPrims.emplace_back(aZero::Helper::Pack8To32(primitives[i], primitives[i + 1], primitives[i + 2], 0));
	}

	std::vector<uint8_t> newPrimsa;
	newPrimsa.reserve(primitives.size() + primitives.size() / 3);

	for (int i = 0; i < primitives.size(); i++)
	{
		newPrimsa.emplace_back(primitives[i]);
		if ((i + 1) % 3 == 0)
		{
			newPrimsa.emplace_back(0);
		}
	}

	return {
		.Name = name,
		.Meshlets = std::move(finalMeshlets),
		.MeshletIndices = std::move(local_indices),
		.MeshletPrimitives = std::move(newPrims),
		.Positions = std::move(newPositions),
		.GenericVertexData = std::move(newGeneric),
		.Bounds = bounds
	};
}

DirectX::BoundingSphere ComputeBoundingSphere(const std::vector<aZero::Asset::VertexPosition>& points)
{
	aZero::Asset::VertexPosition p0 = points[0];

	int i1 = 0;
	float maxDist = 0.0f;

	for (int i = 0; i < points.size(); i++)
	{
		float d = (points[i] - p0).LengthSquared();
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
		float d = (points[i] - points[i1]).LengthSquared();
		if (d > maxDist)
		{
			maxDist = d;
			i2 = i;
		}
	}

	// 4. Initial sphere
	aZero::Asset::VertexPosition center = (points[i1] + points[i2]) * 0.5f;
	float radius = (points[i2] - center).Length();

	// 5. Expand sphere
	for (const auto& p : points)
	{
		aZero::Asset::VertexPosition d = p - center;
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

			std::vector<aZero::Asset::VertexPosition> positions;
			std::vector<aZero::Asset::GenericVertexData> genericVertexData;
			std::vector<aZero::Asset::VertexIndex> indices;

			positions.reserve(mesh->mNumVertices);
			genericVertexData.reserve(mesh->mNumVertices);

			for (int i = 0; i < mesh->mNumVertices; i++)
			{
				aiVector3D tempData = mesh->mVertices[i];
				positions.emplace_back(tempData.x, tempData.y, tempData.z);
				genericVertexData.emplace_back();

				if (mesh->HasTextureCoords(0))
				{
					tempData = mesh->mTextureCoords[0][i];
					genericVertexData[genericVertexData.size() - 1].UV = { tempData.x, tempData.y };
				}

				if (mesh->HasNormals())
				{
					tempData = mesh->mNormals[i];
					tempData.Normalize();
					genericVertexData[genericVertexData.size() - 1].Normal = { tempData.x, tempData.y, tempData.z };
				}

				if (mesh->HasTangentsAndBitangents())
				{
					tempData = mesh->mTangents[i];
					tempData.Normalize();
					genericVertexData[genericVertexData.size() - 1].Tangent = { tempData.x, tempData.y, tempData.z };
				}
			}

			indices.reserve(mesh->mNumFaces * 3);
			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				const aiFace& face = mesh->mFaces[i];

				indices.emplace_back(face.mIndices[0]);
				indices.emplace_back(face.mIndices[1]);
				indices.emplace_back(face.mIndices[2]);
			}

			output.emplace_back(GenerateMeshletData(mesh->mName.C_Str(), positions, genericVertexData, indices, ComputeBoundingSphere(positions)));
		}
	}
	return output;
}

std::vector<aZero::Asset::MeshletMeshData> aZero::Asset::LoadFromFile(const std::string& filename)
{
	const std::string absolutePath = PROJECT_DIRECTORY + std::string("assets/meshes/") + filename;

	const size_t lastDot = absolutePath.find_last_of('.');
	const std::string suffix = absolutePath.substr(lastDot + 1, absolutePath.length() - (absolutePath.length() - lastDot));
	if (suffix == "fbx")
	{
		return LoadFBX(absolutePath);
	}

	// Add other formats here...

	return std::vector<aZero::Asset::MeshletMeshData>();
}

bool aZero::Asset::Mesh::LoadFromFile(const std::string& filename)
{
	const auto& meshes = Asset::LoadFromFile(filename);
	if (meshes.size())
	{

		m_VertexData = meshes[0];
	}

	return !meshes.empty();
}