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

	namespace Scene
	{
		struct RenderData;
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
			std::string Name;
			std::vector<Meshlet> Meshlets;
			std::vector<Vertex> Vertices;
			std::vector<uint32_t> Primitives;
			std::vector<Index> Indices;
			DirectX::BoundingSphere Bounds;
		};

		std::vector<MeshletMeshData> LoadFromFile(const std::string& filePath);

		struct Submesh
		{
			std::string Name;
			uint32_t MeshletOffset, MeshletCount;
			uint32_t PrimitiveOffset, VertexOffset, IndexOffset;
			DirectX::BoundingSphere Bounds;
		};

		class Mesh : public AssetBase
		{
			friend struct Scene::RenderData;
		public:
			Mesh() = default;
			Mesh(const FBX::FBX_Mesh& mesh);

			const MeshletMeshData& GetVertexData() const { return m_VertexData; }

			bool LoadFromFile(const std::string& filePath);

			std::vector<Submesh> m_Submeshes;
		private:
			MeshletMeshData m_VertexData;


		};
	}
}