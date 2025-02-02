#include "RenderAssetData.hpp"
#include "misc/EngineDebugMacros.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "WICTextureLoader.h"
#include "misc/stb_image.h"

bool aZero::Asset::MeshData::LoadFromFile(const std::string& FilePath)
{
	Assimp::Importer Importer;
	const aiScene* const Scene = Importer.ReadFile(FilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!Scene)
	{
		DEBUG_PRINT("Failed to load mesh: " + FilePath);
		return false;
	}

	if (Scene->mNumMeshes > 0)
	{
		const aiMesh* const NewAssimpMesh = Scene->mMeshes[0];
		Name.assign(NewAssimpMesh->mName.C_Str());

		BoundingSphereRadius = 0.0;

		Vertices.resize(0);
		Vertices.reserve(NewAssimpMesh->mNumVertices);

		for (unsigned int i = 0; i < NewAssimpMesh->mNumVertices; i++)
		{
			VertexData Vertex;
			aiVector3D TempData;
			TempData = NewAssimpMesh->mVertices[i];
			Vertex.position = { TempData.x, TempData.y, TempData.z };

			if (NewAssimpMesh->HasTextureCoords(0))
			{
				TempData = NewAssimpMesh->mTextureCoords[0][i];
				Vertex.uv = { TempData.x, TempData.y };
			}
			else
			{
				Vertex.uv = { 0,0 };
			}

			if (NewAssimpMesh->HasNormals())
			{
				TempData = NewAssimpMesh->mNormals[i];
				TempData.Normalize();
				Vertex.normal = { TempData.x, TempData.y, TempData.z };
			}
			else
			{
				Vertex.normal = { 0, 0, 0 };
			}

			if (NewAssimpMesh->HasTangentsAndBitangents())
			{
				TempData = NewAssimpMesh->mTangents[i];
				TempData.Normalize();
				Vertex.tangent = { TempData.x, TempData.y, TempData.z };
			}
			else
			{
				Vertex.tangent = { 0, 0, 0 };
			}

			if (Vertex.position.Length() > BoundingSphereRadius)
			{
				BoundingSphereRadius = Vertex.position.Length();
			}

			Vertices.emplace_back(std::move(Vertex));
		}
		
		Indices.resize(0);
		for (unsigned int i = 0; i < NewAssimpMesh->mNumFaces; i++)
		{
			Indices.reserve(Indices.capacity() + NewAssimpMesh->mFaces->mNumIndices); // capacity???? why
			for (int h = 0; h < NewAssimpMesh->mFaces[i].mNumIndices; h++)
			{
				Indices.emplace_back(NewAssimpMesh->mFaces[i].mIndices[h]);
			}
		}
	}

	return true;
}

bool aZero::Asset::TextureData::LoadFromFile(const std::string& FilePath, DXGI_FORMAT Format)
{
	std::int32_t Width, Height, Channels;
	stbi_uc* LoadedImage = stbi_load(FilePath.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
	if (!LoadedImage)
	{
		DEBUG_PRINT("Failed to load file: " + FilePath);
		return false;
	}

	if (Channels == 4 || Channels == 3)
	{
		m_Data.resize(Width * Height * 4);
		memcpy(m_Data.data(), LoadedImage, m_Data.size());
		m_Dimensions.x = Width;
		m_Dimensions.y = Height;
		m_Format = Format;
	}
	else
	{
		DEBUG_PRINT("Loading a texture with less than 3 channels isn't supported");
		return false;
	}

	stbi_image_free(LoadedImage);

	return true;
}
