#pragma once
#include "Core/Renderer/D3D12Wrap/Resources/StructuredBuffer.h"

namespace aZero
{
	namespace Asset
	{
		struct DefaultVertexType
		{
			DXM::Vector3 position;
			DXM::Vector2 uv;
			DXM::Vector3 normal;
			DXM::Vector3 tangent;
		};

		struct MeshInfo
		{
			std::string Name = "";
			std::vector<DefaultVertexType> Vertices;
			std::vector<UINT> Indices;
			double BoundingSphereRadius = 0.0;
		};

		bool LoadFBXFile(std::vector<MeshInfo>& OutMeshes, const std::string& FilePath);

		class MeshAsset
		{
		private:
			std::string m_Name;

			D3D12::DefaultHeapStructuredBuffer<DefaultVertexType> m_VertexBuffer;
			D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

			D3D12::DefaultHeapStructuredBuffer<UINT> m_IndexBuffer;
			D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

			UINT m_NumVertices;
			UINT m_NumIndices;
			double m_BoundingSphereRadius = 0.0;

			void CopyMove(MeshAsset& Other)
			{
				m_Name = Other.m_Name;

				m_VertexBuffer = std::move(Other.m_VertexBuffer);
				m_VertexBufferView = Other.m_VertexBufferView;

				m_IndexBuffer = std::move(Other.m_IndexBuffer);
				m_IndexBufferView = Other.m_IndexBufferView;

				m_NumVertices = Other.m_NumVertices;
				m_NumIndices = Other.m_NumIndices;
				m_BoundingSphereRadius = Other.m_BoundingSphereRadius;
			}

		public:
			MeshAsset() = default;

			MeshAsset(const MeshInfo& Info, ID3D12GraphicsCommandList* CmdList)
			{
				this->Init(Info, CmdList);
			}

			MeshAsset(const MeshAsset& Other) = delete;
			MeshAsset operator= (const MeshAsset&) = delete;

			MeshAsset(MeshAsset&& Other) noexcept
			{
				this->CopyMove(Other);
			}

			MeshAsset& operator= (MeshAsset&& Other) noexcept
			{
				if (this != &Other)
				{
					this->CopyMove(Other);
				}

				return *this;
			}

			void Init(const MeshInfo& Info, ID3D12GraphicsCommandList* CmdList)
			{
				m_Name = Info.Name;
				m_NumVertices = Info.Vertices.size();
				m_NumIndices = Info.Indices.size();
				m_BoundingSphereRadius = Info.BoundingSphereRadius;

				D3D12::UploadHeapStructuredBuffer<DefaultVertexType> StagingBufferVertices(m_NumVertices, DXGI_FORMAT_UNKNOWN, &gResourceRecycler);
				D3D12::UploadHeapStructuredBuffer<UINT> StagingBufferIndices(m_NumIndices, DXGI_FORMAT_UNKNOWN, &gResourceRecycler);
				
				m_VertexBuffer.Init(m_NumVertices, DXGI_FORMAT_UNKNOWN, &gResourceRecycler);
				m_IndexBuffer.Init(m_NumIndices, DXGI_FORMAT_UNKNOWN, &gResourceRecycler);

				m_VertexBuffer.Write(0, m_NumVertices, (void*)Info.Vertices.data(), StagingBufferVertices.GetInternalBuffer(), 0, CmdList);
				m_IndexBuffer.Write(0, m_NumIndices, (void*)Info.Indices.data(), StagingBufferIndices.GetInternalBuffer(), 0, CmdList);

				m_VertexBufferView.BufferLocation = m_VertexBuffer.GetInternalBuffer().GetResource()->GetGPUVirtualAddress();
				m_VertexBufferView.SizeInBytes = m_NumVertices * sizeof(DefaultVertexType);
				m_VertexBufferView.StrideInBytes = sizeof(DefaultVertexType);

				m_IndexBufferView.BufferLocation = m_IndexBuffer.GetInternalBuffer().GetResource()->GetGPUVirtualAddress();
				m_IndexBufferView.SizeInBytes = m_NumIndices * sizeof(UINT);
				m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
			}

			std::string GetName() const { return m_Name; }
			double GetSphericalBounds() const { return m_BoundingSphereRadius; }

			UINT GetNumVertices() const { return m_NumVertices; }
			UINT GetNumIndices() const { return m_NumIndices; }

			D3D12_VERTEX_BUFFER_VIEW GetVertexView() const { return m_VertexBufferView; }
			D3D12_INDEX_BUFFER_VIEW GetIndexView() const { return m_IndexBufferView; }
		};
	}
}