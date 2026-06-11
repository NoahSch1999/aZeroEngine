#pragma once
#include <vector>
#include <array>
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
		static inline constexpr uint32_t g_VerticesPerMeshlet = 64;
		static inline constexpr uint32_t g_PrimitivesPerMeshlet = 84;

		struct Meshlet
		{
			uint32_t VertexOffset;
			uint32_t VertexCount;
			uint32_t PrimitiveCount;
			std::array<uint32_t, g_PrimitivesPerMeshlet> Primitives;
		};

		struct MeshletMeshData
		{
			std::vector<Meshlet> Meshlets;
			std::vector<DirectX::BoundingSphere> MeshletBounds;
			std::vector<DXM::Vector3> Positions;
			std::vector<Vertex> Vertices;
		};

		struct Submesh
		{
			std::string Name;
			uint32_t MeshletOffset, MeshletCount;
			DirectX::BoundingSphere Bounds;
		};

		class Mesh : public AssetBase
		{
			friend Rendering::ResourceManager;
		public:
			Mesh() = default;
			Mesh(const FBX::FBX_Mesh& mesh);

			const MeshletMeshData& GetVertexData() const { return m_VertexData; }
			const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

			// Mesh Info
			uint32_t m_MeshletGlobalOffset;
			uint32_t m_VertexGlobalOffset;
			//
		private:
			std::vector<Submesh> m_Submeshes;
			MeshletMeshData m_VertexData;
		};
	}
}