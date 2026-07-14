#pragma once
#include "MeshData.hpp"

namespace aZero::Asset
{
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

		void ClearCachedData() { 
			m_CachedData.m_VertexData.Meshlets.clear();
			m_CachedData.m_VertexData.MeshletBounds.clear();
			m_CachedData.m_VertexData.Vertices.clear();
		}

		std::pair<uint32_t, std::array<SubmeshData, 10>> GetSubmeshes() const { return std::make_pair(m_NumSubmeshes, m_Submeshes); } // todo Return reference to array

	private:
		std::array<SubmeshData, 10> m_Submeshes;
		uint32_t m_NumSubmeshes = 0;
	};
}