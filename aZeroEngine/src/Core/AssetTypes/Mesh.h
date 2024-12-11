#pragma once
#include "Core/D3D12Include.h"
#include "Core/AssetTypes/RenderFileAsset.h"
#include "Core/DataStructures/FreelistAllocator.h"

namespace aZero
{
	namespace Asset
	{
		struct VertexData
		{
			DXM::Vector3 position;
			DXM::Vector2 uv;
			DXM::Vector3 normal;
			DXM::Vector3 tangent;
		};

		struct MeshData
		{
			std::string Name;
			std::vector<VertexData> Vertices;
			std::vector<uint32_t> Indices;
			double BoundingSphereRadius;
		};

		bool LoadFBXFile(MeshData& OutMesh, const std::string& Path);

		struct MeshGPUHandle
		{
			DS::FreelistAllocator::AllocationHandle m_VertexBufferAllocHandle;
			DS::FreelistAllocator::AllocationHandle m_IndexBufferAllocHandle;
			DS::FreelistAllocator::AllocationHandle m_MeshEntryAllocHandle;
		};

		struct MeshGPUEntry
		{
			uint32_t m_VertexStartOffset;
			uint32_t m_IndexStartOffset;
			uint32_t m_NumIndices;
		};

		class Mesh : public RenderFileAsset<MeshData, MeshGPUHandle>
		{
		private:

		public:
			Mesh() = default;

			~Mesh()
			{
				if (this->HasRenderState())
				{
					if (m_AssetGPUHandle.use_count() == 1)
					{
						this->RemoveRenderState();
					}
				}
			}

			virtual bool HasRenderState() const override
			{
				if (m_AssetGPUHandle->m_VertexBufferAllocHandle.IsValid())
				{
					return true;
				}

				return false;
			}

			virtual bool LoadFromFile(const std::string& Path) override
			{
				if (true /* TODO: Add support for other file formats*/)
				{
					if (!Path.ends_with(".fbx"))
					{
						DEBUG_PRINT("Mesh::LoadFromFile() => Path extension isn't .fbx");
						return false;
					}

					MeshData NewMesh;
					if (LoadFBXFile(NewMesh, Path))
					{
						this->SetAssetData(std::move(NewMesh));
					}
				}

				return true;
			}
		};
	}
}