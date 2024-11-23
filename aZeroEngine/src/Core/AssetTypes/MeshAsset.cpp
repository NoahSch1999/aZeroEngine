#include "MeshAsset.h"
#include "ThirdParty/assimp/include/assimp/Importer.hpp"
#include "ThirdParty/assimp/include/assimp/scene.h"
#include "ThirdParty/assimp/include/assimp/postprocess.h"

bool aZero::Asset::LoadFBXFile(std::vector<MeshInfo>& OutMeshes, const std::string& FilePath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(FilePath, aiProcess_SortByPType | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (scene)
	{
		for (int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
		{
			OutMeshes.push_back(MeshInfo());
			MeshInfo& newMesh = OutMeshes[OutMeshes.size() - 1];
			newMesh.Name.assign(scene->mMeshes[meshIndex]->mName.C_Str());

			float BoundingSphereRadius = 0.0;

			newMesh.Vertices.reserve(scene->mMeshes[meshIndex]->mNumVertices);
			for (unsigned int i = 0; i < scene->mMeshes[meshIndex]->mNumVertices; i++)
			{
				DefaultVertexType vertex;
				aiVector3D tempData;
				tempData = scene->mMeshes[meshIndex]->mVertices[i];
				vertex.position = { tempData.x, tempData.y, tempData.z };

				tempData = scene->mMeshes[meshIndex]->mTextureCoords[0][i];
				vertex.uv = { tempData.x, tempData.y };

				tempData = scene->mMeshes[meshIndex]->mNormals[i];
				tempData.Normalize();
				vertex.normal = { tempData.x, tempData.y, tempData.z };

				tempData = scene->mMeshes[meshIndex]->mTangents[i];
				tempData.Normalize();
				vertex.tangent = { tempData.x, tempData.y, tempData.z };

				newMesh.Vertices.emplace_back(std::move(vertex));

				if (vertex.position.Length() > BoundingSphereRadius)
				{
					BoundingSphereRadius = vertex.position.Length();
				}
			}

			for (unsigned int i = 0; i < scene->mMeshes[meshIndex]->mNumFaces; i++)
			{
				newMesh.Indices.reserve(newMesh.Indices.capacity() + scene->mMeshes[meshIndex]->mFaces->mNumIndices);
				for (int h = 0; h < scene->mMeshes[meshIndex]->mFaces[i].mNumIndices; h++)
				{
					newMesh.Indices.emplace_back(scene->mMeshes[meshIndex]->mFaces[i].mIndices[h]);
				}
			}

			newMesh.BoundingSphereRadius = BoundingSphereRadius;
		}
	}
	return scene != nullptr;
}