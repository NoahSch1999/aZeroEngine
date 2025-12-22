/**
 * @file Mesh.h
 * @brief Defines the Mesh asset class.
 *
 * @date 2025-11-04
 * @version 1.0
 */

#pragma once
#include <vector>
#include "Asset.hpp"

namespace aZero
{
	namespace Asset
	{
		using VertexIndex = uint32_t;

		/**
		 * @class Mesh
		 * @brief Represents a 3D mesh asset containing vertex and index data.
		 *
		 * The `Mesh` class derives from `AssetBase` and encapsulates all the vertex-level
		 * data required for rendering. It stores vertex attributes such as positions,
		 * UVs, normals, and tangents, as well as index data. 
		 * Mesh data can be loaded from an external file using the `Load()`.
		 * 
		 * Instances are managed by the AssetManager class.
		 */
		class Mesh : public AssetBase
		{
		private:
			friend class AssetAllocator<Mesh>;
			Mesh() = default;
			Mesh(AssetID assetID, const std::string& name)
				:AssetBase(assetID, name)
			{

			}
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

		class NewMesh : public AssetBase
		{
		private:
			friend class NewAssetAllocator<NewMesh>;
			NewMesh() = default;
			NewMesh(AssetID assetID, const std::string& name)
				:AssetBase(assetID, name)
			{

			}
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