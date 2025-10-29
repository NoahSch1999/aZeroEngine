#include <vector>
#include "Asset.hpp"

#pragma once

namespace aZero
{
	namespace AssetNew
	{
		using VertexIndex = uint32_t;

		class Mesh : public AssetBase
		{
		private:
			friend class AssetAllocator<Mesh>;
			Mesh() = default;
		public:
			struct VertexData
			{
				std::vector<DXM::Vector3> Positions;
				std::vector<DXM::Vector2> UVs;
				std::vector<DXM::Vector3> Normals;
				std::vector<DXM::Vector3> Tangents;
				std::vector<VertexIndex>  Indices;
			};

			bool Load(const std::string& filePath);

			void RemoveVertexData()
			{
				m_VertexData.Positions.clear();
				m_VertexData.UVs.clear();
				m_VertexData.Normals.clear();
				m_VertexData.Tangents.clear();
				m_VertexData.Indices.clear();
			}

			VertexData m_VertexData;
			float m_BoundingSphereRadius;
		};
	}
}