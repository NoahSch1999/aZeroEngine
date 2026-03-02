#include "Mesh.hpp"
#include "misc/EngineDebugMacros.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

bool aZero::Asset::Mesh::Load(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* const scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene)
	{
		DEBUG_PRINT("Failed to load mesh at path: " + path);
		return false;
	}

	if (scene->mNumMeshes > 0)
	{
		const aiMesh* const newAssimpMesh = scene->mMeshes[0];

		m_BoundingSphereRadius = 0.0;

		m_VertexData.Positions.reserve(newAssimpMesh->mNumVertices);
		m_VertexData.UVs.reserve(newAssimpMesh->mNumVertices);
		m_VertexData.Normals.reserve(newAssimpMesh->mNumVertices);
		m_VertexData.Tangents.reserve(newAssimpMesh->mNumVertices);

		for (int i = 0; i < newAssimpMesh->mNumVertices; i++)
		{
			aiVector3D tempData;
			tempData = newAssimpMesh->mVertices[i];
			m_VertexData.Positions.emplace_back(tempData.x, tempData.y, tempData.z);

			if (newAssimpMesh->HasTextureCoords(0))
			{
				tempData = newAssimpMesh->mTextureCoords[0][i];
				m_VertexData.UVs.emplace_back(tempData.x, tempData.y);
			}

			if (newAssimpMesh->HasNormals())
			{
				tempData = newAssimpMesh->mNormals[i];
				tempData.Normalize();
				m_VertexData.Normals.emplace_back(tempData.x, tempData.y, tempData.z);
			}

			if (newAssimpMesh->HasTangentsAndBitangents())
			{
				tempData = newAssimpMesh->mTangents[i];
				tempData.Normalize();
				m_VertexData.Tangents.emplace_back(tempData.x, tempData.y, tempData.z);
			}

			float originToVertexDistance = m_VertexData.Positions.at(i).Length();
			if (originToVertexDistance > m_BoundingSphereRadius)
			{
				m_BoundingSphereRadius = originToVertexDistance;
			}
		}

		m_VertexData.Indices.reserve(newAssimpMesh->mNumVertices);
		for (unsigned int i = 0; i < newAssimpMesh->mNumFaces; i++)
		{
			for (int h = 0; h < newAssimpMesh->mFaces[i].mNumIndices; h++)
			{
				m_VertexData.Indices.emplace_back(newAssimpMesh->mFaces[i].mIndices[h]);
			}
		}
	}

	return true;
}

void aZero::Asset::Mesh::RemoveVertexData()
{
	m_VertexData.Positions.clear();
	m_VertexData.UVs.clear();
	m_VertexData.Normals.clear();
	m_VertexData.Tangents.clear();
	m_VertexData.Indices.clear();
}