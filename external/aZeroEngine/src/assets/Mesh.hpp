#pragma once
#include <vector>
#include "Asset.hpp"

namespace aZero
{
	namespace Scene
	{
		struct RenderData;
	}

	namespace Asset
	{
		using VertexIndex = uint32_t;

		struct Meshlet
		{
			uint32_t VertexOffset;
			uint32_t VertexCount;
			uint32_t PrimitiveOffset;
			uint32_t PrimitiveCount;
			DirectX::BoundingSphere Bounds;
		};

		struct Vertex
		{
			DXM::Vector3 Position;
			DXM::Vector2 UV;
			DXM::Vector3 Normal;
			DXM::Vector3 Tangent;
		};

		struct MeshletMeshData
		{
			std::string Name;
			std::vector<Meshlet> Meshlets;
			std::vector<Vertex> Vertices;
			std::vector<uint8_t> Primitives;
			DirectX::BoundingSphere Bounds;
		};

		std::vector<MeshletMeshData> LoadFromFile(const std::string& filePath);

		class Mesh : public AssetBase
		{
			friend struct Scene::RenderData;
		public:
			Mesh() = default;

			const MeshletMeshData& GetVertexData() const { return m_VertexData; }

			bool LoadFromFile(const std::string& filePath);
		private:
			MeshletMeshData m_VertexData;
		};
	}
}