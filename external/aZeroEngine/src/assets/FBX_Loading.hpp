#pragma once
#include <vector>
#include <optional>
#include <stack>
#include <stdfloat>
#include <string_view>
#include "misc/HelperFunctions.hpp"
#include "MeshPrimitives.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace aZero::FBX
{
	struct FBX_Material
	{
		std::string Name;
		std::string Albedo;
		std::string NormalMap;
		std::string EmissiveMap;
		std::string TransparencyMap;
	};

	struct FBX_Submesh
	{
		std::string Name;
		DirectX::BoundingSphere Bounds;
		FBX_Material Material;
		std::vector<DXM::Vector3> Positions;
		std::vector<Asset::Vertex> Vertices;
		std::vector<uint32_t> Indices;
	};

	struct FBX_Mesh
	{
		std::vector<FBX_Submesh> Submeshes;
	};

	struct FBX_FileData
	{
		std::vector<FBX_Mesh> Meshes;
	};

	inline std::optional<FBX_FileData> LoadFBX(std::string_view path)
	{
		Assimp::Importer importer;
		const aiScene* const scene = importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices /*| aiProcess_CalcTangentSpace*/);
		if (!scene)
		{
			return {};
		}

		FBX_FileData output;

		std::stack<aiNode*> currentNode;
		currentNode.push(scene->mRootNode);

		while (!currentNode.empty())
		{
			aiNode* node = currentNode.top();
			currentNode.pop();

			if (node->mNumMeshes)
			{
				output.Meshes.push_back(FBX_Mesh());
			}

			for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; meshIndex++)
			{
				FBX_Submesh submesh;

				// Geometry processing
				const aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];
				submesh.Name = mesh->mName.C_Str();

				submesh.Vertices.reserve(mesh->mNumVertices);
				submesh.Positions.reserve(mesh->mNumVertices);
				submesh.Indices.reserve(mesh->mNumFaces * 3);

				for (uint32_t i = 0; i < mesh->mNumVertices; i++)
				{
					Asset::Vertex vertex = Asset::PackVertex(
						Helper::EncodeNormalOctahedral({ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z }),
						{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y }
					);
					submesh.Vertices.emplace_back(vertex);
					submesh.Positions.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
				}

				for (uint32_t i = 0; i < mesh->mNumFaces; i++)
				{
					const aiFace& face = mesh->mFaces[i];
					submesh.Indices.emplace_back(face.mIndices[0]);
					submesh.Indices.emplace_back(face.mIndices[1]);
					submesh.Indices.emplace_back(face.mIndices[2]);
				}

				submesh.Bounds = Helper::ComputeBoundingSphere(submesh.Positions);

				// Material processing
				const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
				submesh.Material.Name = material->GetName().C_Str();

				aiString texturePath;
				aiReturn texRet = material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
				if(texRet == aiReturn_SUCCESS) { submesh.Material.Albedo = texturePath.C_Str(); }
				texRet = material->GetTexture(aiTextureType_NORMALS, 0, &texturePath);
				if (texRet == aiReturn_SUCCESS) { submesh.Material.NormalMap = texturePath.C_Str(); }
				texRet = material->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath);
				if (texRet == aiReturn_SUCCESS) { submesh.Material.EmissiveMap = texturePath.C_Str(); }
				texRet = material->GetTexture(aiTextureType_OPACITY, 0, &texturePath);
				if (texRet == aiReturn_SUCCESS) { submesh.Material.TransparencyMap = texturePath.C_Str(); }

				output.Meshes[output.Meshes.size() - 1].Submeshes.emplace_back(std::move(submesh));
			}

			for (uint32_t i = 0; i < node->mNumChildren; i++)
			{
				currentNode.push(node->mChildren[i]);
			}
		}

		return output;
	}
}
