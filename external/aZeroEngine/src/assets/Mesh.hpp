#pragma once
#include <vector>
#include "Asset.hpp"
#include "Vertex.hpp"

namespace aZero
{
	namespace FBX
	{
		struct FBX_Mesh;
	}

	namespace Asset
	{
		struct Meshlet
		{
			uint32_t VertexOffset;
			uint32_t VertexCount;
			uint32_t PrimitiveOffset;
			uint32_t PrimitiveCount;
			DirectX::BoundingSphere Bounds;
		};

		struct MeshletMeshData
		{
			std::vector<Meshlet> Meshlets;
			std::vector<Vertex> Vertices;
			std::vector<uint32_t> Primitives;
			std::vector<Index> Indices;
		};

		struct Submesh
		{
			std::string Name;
			uint32_t MeshletOffset, MeshletCount;
			DirectX::BoundingSphere Bounds;
		};

		class Mesh : public AssetBase
		{
		public:
			Mesh() = default;
			Mesh(const FBX::FBX_Mesh& mesh);

			const MeshletMeshData& GetVertexData() const { return m_VertexData; }
			const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		private:
			std::vector<Submesh> m_Submeshes;
			MeshletMeshData m_VertexData;
		};
	}
}