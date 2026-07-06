#pragma once
#include "RenderAssetBase.hpp"
#include "FBX_Loading.hpp"

namespace aZero::Asset
{
	struct MeshletMeshData
	{
		std::vector<aZero::Asset::Meshlet> Meshlets;
		std::vector<DirectX::BoundingSphere> MeshletBounds;
		std::vector<DXM::Vector3> Positions;
		std::vector<aZero::Asset::Vertex> Vertices;
	};

	struct SubmeshData
	{
		std::string Name;
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
		MeshData(const FBX::FBX_Mesh& mesh)
		{
			uint32_t vertexOffset = 0;
			for (const FBX::FBX_Submesh& submesh : mesh.Submeshes)
			{
				std::vector<Asset::Vertex> vertices(submesh.Vertices);
				std::vector<DXM::Vector3> positions(submesh.Positions);
				std::vector<Asset::Index> indices(submesh.Indices);
				std::vector<Asset::Meshlet> meshlets;
				std::vector<DirectX::BoundingSphere> meshletBounds;
				Asset::Meshletize(positions, vertices, indices, meshlets, meshletBounds, vertexOffset);

				SubmeshData newSubmesh;
				newSubmesh.Name = submesh.Name;
				newSubmesh.Bounds = submesh.Bounds;
				newSubmesh.MeshletOffset = m_VertexData.Meshlets.size();
				newSubmesh.MeshletCount = meshlets.size();
				vertexOffset += positions.size();

				m_VertexData.Positions.insert(m_VertexData.Positions.end(), positions.begin(), positions.end());
				m_VertexData.Vertices.insert(m_VertexData.Vertices.end(), vertices.begin(), vertices.end());
				m_VertexData.Meshlets.insert(m_VertexData.Meshlets.end(), meshlets.begin(), meshlets.end());
				m_VertexData.MeshletBounds.insert(m_VertexData.MeshletBounds.end(), meshletBounds.begin(), meshletBounds.end());

				m_Submeshes.push_back(newSubmesh);
			}
		}
	};

	struct MeshRenderRef
	{
		uint32_t m_MeshletGlobalOffset = std::numeric_limits<uint32_t>::max();
		uint32_t m_VertexGlobalOffset = std::numeric_limits<uint32_t>::max();

		bool IsValid() const {
			return m_MeshletGlobalOffset != std::numeric_limits<uint32_t>::max() && m_VertexGlobalOffset != std::numeric_limits<uint32_t>::max();
		}
	};

	class Mesh : public RenderAssetBase<MeshRenderRef, MeshData>
	{
	public:
		Mesh() = default;
		Mesh(MeshData&& data)
			:RenderAssetBase(std::move(data))
		{
			const auto& cachedData = this->GetCachedData();
			for (const auto& mesh : cachedData.m_Submeshes)
			{
				m_Submeshes[m_NumSubmeshes] = mesh;
				m_NumSubmeshes++;
			}
		}

		Mesh(const MeshData& data)
			:RenderAssetBase(data)
		{
			const auto& cachedData = this->GetCachedData();
			for (const auto& mesh : cachedData.m_Submeshes)
			{
				m_Submeshes[m_NumSubmeshes] = mesh;
				m_NumSubmeshes++;
			}
		}

		std::pair<uint32_t, std::array<SubmeshData, 10>> GetSubmeshes() const { return std::make_pair(m_NumSubmeshes, m_Submeshes); } // todo Return reference to array

	private:
		std::array<SubmeshData, 10> m_Submeshes;
		uint32_t m_NumSubmeshes = 0;
	};
}