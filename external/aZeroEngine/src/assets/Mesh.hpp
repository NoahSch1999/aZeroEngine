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
			uint32_t VerticesCount;
			uint32_t VertexOffset;
			uint32_t PrimitivesCount;
			uint32_t PrimitiveOffset;
			DirectX::BoundingSphere Bounds;
		};

		using VertexPosition = DXM::Vector3;

		struct GenericVertexData
		{
			DXM::Vector2 UV;
			DXM::Vector3 Normal;
			DXM::Vector3 Tangent; // TODO: Omit and calc it in the shader
		};

		struct MeshletMeshData
		{
			std::string Name;
			std::vector<Meshlet> Meshlets;
			std::vector<VertexIndex> MeshletIndices;
			std::vector<uint8_t> MeshletPrimitives;
			std::vector<VertexPosition> Positions;
			std::vector<GenericVertexData> GenericVertexData;
			DirectX::BoundingSphere Bounds;
		};

		std::vector<MeshletMeshData> LoadFromFile(const std::string& filename);

		class Mesh : public AssetBase
		{
			friend struct Scene::RenderData;
		public:
			Mesh() = default;
			Mesh(AssetID id)
				:AssetBase(id) { }

			const MeshletMeshData& GetVertexData() const { return m_VertexData; }

			bool LoadFromFile(const std::string& filename);
		private:
			MeshletMeshData m_VertexData;
		};
	}
}