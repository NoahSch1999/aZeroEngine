#pragma once
#include "RenderAssetBase.hpp"
#include "FBX_Loading.hpp"

namespace aZero::Asset
{
	struct MeshletMeshData
	{
		std::vector<aZero::Asset::Meshlet> Meshlets;
		std::vector<DirectX::BoundingSphere> MeshletBounds;
		std::vector<aZero::Asset::Vertex> Vertices;
	};

	struct SubmeshData
	{
		uint32_t MeshletOffset, MeshletCount;
		DirectX::BoundingSphere Bounds;
	};

	struct MeshData
	{
		std::string Name;
		std::string FilePath;
		std::vector<SubmeshData> m_Submeshes;
		MeshletMeshData m_VertexData;

		MeshData() = default;

		// todo Implement init using gltf

		MeshData(const FBX::FBX_Mesh& mesh)
		{
			uint32_t vertexOffset = 0;
			for (const FBX::FBX_Submesh& submesh : mesh.Submeshes)
			{
				std::vector<Asset::Vertex> vertices(submesh.Vertices);
				std::vector<Asset::Index> indices(submesh.Indices);
				std::vector<Asset::Meshlet> meshlets;
				std::vector<DirectX::BoundingSphere> meshletBounds;
				Asset::Meshletize(vertices, indices, meshlets, meshletBounds, vertexOffset);

				SubmeshData newSubmesh;
				newSubmesh.Bounds = submesh.Bounds;
				newSubmesh.MeshletOffset = m_VertexData.Meshlets.size();
				newSubmesh.MeshletCount = meshlets.size();
				vertexOffset += vertices.size();

				m_VertexData.Vertices.insert(m_VertexData.Vertices.end(), vertices.begin(), vertices.end());
				m_VertexData.Meshlets.insert(m_VertexData.Meshlets.end(), meshlets.begin(), meshlets.end());
				m_VertexData.MeshletBounds.insert(m_VertexData.MeshletBounds.end(), meshletBounds.begin(), meshletBounds.end());

				m_Submeshes.push_back(newSubmesh);
			}

			if (m_Submeshes.size()) {
				Name = mesh.Name;
			}
		}
	};
}