#pragma once
#include "NEW_RenderAssetBase.hpp"
#include "FBX_Loading.hpp"

namespace aZero
{
	namespace NEW_Asset
	{
		struct MeshletMeshData
		{
			std::vector<Asset::Meshlet> Meshlets;
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
			std::string FilePath;
			std::vector<SubmeshData> m_Submeshes;
			MeshletMeshData m_VertexData;

			MeshData() = default;
			MeshData(const FBX::FBX_Mesh& mesh)
			{
				for (const FBX::FBX_Submesh& submesh : mesh.Submeshes)
				{
					std::vector<Asset::Vertex> vertices(submesh.Vertices);
					std::vector<DXM::Vector3> positions(submesh.Positions);
					std::vector<Asset::Index> indices(submesh.Indices);
					std::vector<Asset::Meshlet> meshlets;
					std::vector<DirectX::BoundingSphere> meshletBounds;
					Asset::Meshletize(positions, vertices, indices, meshlets, meshletBounds);

					SubmeshData newSubmesh;
					newSubmesh.Name = submesh.Name;
					newSubmesh.Bounds = submesh.Bounds;
					newSubmesh.MeshletOffset = m_VertexData.Meshlets.size();
					newSubmesh.MeshletCount = meshlets.size();

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
			
			}

			std::pair<uint32_t, std::reference_wrapper<std::array<SubmeshData, 10>>> GetSubmeshes() { return { m_NumSubmeshes, m_Submeshes }; }

		private:
			std::array<SubmeshData, 10> m_Submeshes;
			uint32_t m_NumSubmeshes = 0;
		};
	}

	template<>
	void aZero::Rendering::Renderer::RegisterOrUpdateAsset<NEW_Asset::Mesh>(NEW_Asset::Mesh& mesh)
	{
		// todo Impl update of existing asset
		if (mesh.GetRenderRef().IsValid())
		{
			throw;
		}
		FrameContext& context = this->GetCurrentContext();
		const auto [meshletOffset, vertexOffset] = m_RenderAssetManager->UpdateRenderState(context.GetFrameStagingAllocator(), 
			mesh.GetCachedData().m_VertexData.Meshlets, mesh.GetCachedData().m_VertexData.Vertices, 
			mesh.GetCachedData().m_VertexData.Positions, mesh.GetCachedData().m_VertexData.MeshletBounds);
		mesh.m_RenderRef.m_MeshletGlobalOffset = meshletOffset;
		mesh.m_RenderRef.m_VertexGlobalOffset = vertexOffset;
	}

	template<>
	inline void aZero::Rendering::Renderer::UnregisterAsset<NEW_Asset::Mesh>(NEW_Asset::Mesh& mesh)
	{
		m_RenderAssetManager->RemoveMeshAsset(mesh.GetRenderRef().m_MeshletGlobalOffset);
	}

	template<>
	inline void aZero::Scene::Scene::OnAssetErased<NEW_Asset::Mesh>(NEW_Asset::Mesh& mesh)
	{
		m_World.defer_begin();
		m_World.query<Component::Mesh>().each([this, mesh](flecs::entity entity, const Component::Mesh& meshComponent) {
			if (meshComponent.m_MeshID == mesh.GetRenderRef().m_MeshletGlobalOffset)
			{
				entity.remove<Component::Mesh>();
			}
		});
		m_World.defer_end();
	}
}