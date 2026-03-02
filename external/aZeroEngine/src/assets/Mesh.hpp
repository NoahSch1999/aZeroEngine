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

		class Mesh : public AssetBase
		{
			friend struct Scene::RenderData;
		public:
			struct Data
			{
				std::vector<DXM::Vector3> Positions;
				std::vector<DXM::Vector2> UVs;
				std::vector<DXM::Vector3> Normals;
				std::vector<DXM::Vector3> Tangents;
				std::vector<VertexIndex>  Indices;
			};

			Mesh() = default;
			Mesh(const std::string& name)
				:AssetBase(name) { }

			bool Load(const std::string& filePath);

			void RemoveVertexData();

			const Data& GetVertexData() const { return m_VertexData; }
			float GetBoundingSphereRadius() const { return m_BoundingSphereRadius; }

		private:
			Data m_VertexData;
			float m_BoundingSphereRadius;
		};
	}
}