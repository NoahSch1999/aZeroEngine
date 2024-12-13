#include "Mesh.h"
#include "ThirdParty/assimp/include/assimp/Importer.hpp"
#include "ThirdParty/assimp/include/assimp/scene.h"
#include "ThirdParty/assimp/include/assimp/postprocess.h"

bool aZero::Asset::LoadFBXFile(const std::string& Path, MeshData& OutMesh)
{
	Assimp::Importer Importer;
	const aiScene* const Scene = Importer.ReadFile(Path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (Scene)
	{
		if (Scene->mNumMeshes > 0)
		{
			const aiMesh* const NewAssimpMesh = Scene->mMeshes[0];
			OutMesh.Name.assign(NewAssimpMesh->mName.C_Str());

			double BoundingSphereRadius = 0.0;
			OutMesh.Vertices.reserve(NewAssimpMesh->mNumVertices);

			for (unsigned int i = 0; i < NewAssimpMesh->mNumVertices; i++)
			{
				VertexData Vertex;
				aiVector3D TempData;
				TempData = NewAssimpMesh->mVertices[i];
				Vertex.position = { TempData.x, TempData.y, TempData.z };

				TempData = NewAssimpMesh->mTextureCoords[0][i];
				Vertex.uv = { TempData.x, TempData.y };

				TempData = NewAssimpMesh->mNormals[i];
				TempData.Normalize();
				Vertex.normal = { TempData.x, TempData.y, TempData.z };

				TempData = NewAssimpMesh->mTangents[i];
				TempData.Normalize();
				Vertex.tangent = { TempData.x, TempData.y, TempData.z };

				if (Vertex.position.Length() > BoundingSphereRadius)
				{
					BoundingSphereRadius = Vertex.position.Length();
				}

				OutMesh.Vertices.emplace_back(std::move(Vertex));
			}

			for (unsigned int i = 0; i < NewAssimpMesh->mNumFaces; i++)
			{
				OutMesh.Indices.reserve(OutMesh.Indices.capacity() + NewAssimpMesh->mFaces->mNumIndices); // capacity???? why
				for (int h = 0; h < NewAssimpMesh->mFaces[i].mNumIndices; h++)
				{
					OutMesh.Indices.emplace_back(NewAssimpMesh->mFaces[i].mIndices[h]);
				}
			}

			OutMesh.BoundingSphereRadius = BoundingSphereRadius;
		}
	}
	return Scene != nullptr;
}
