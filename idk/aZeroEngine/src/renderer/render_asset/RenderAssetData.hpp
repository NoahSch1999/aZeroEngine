#pragma once
#include "RenderGarbageCollector.hpp"
#include "graphics_api/D3D12Include.hpp"
#include "misc/FreelistAllocator.hpp"
#include "graphics_api/resource_type/GPUResource.hpp"
#include "graphics_api/resource_type/GPUResourceView.hpp"

namespace aZero
{
	namespace Asset
	{
		template<typename AssetType, typename GPUHandleType>
		class RenderAsset;

		struct MeshEntry
		{
			struct GPUData
			{
				std::uint32_t VertexStartOffset;
				std::uint32_t IndexStartOffset;
				std::uint32_t NumIndices;
			};
			DS::FreelistAllocator::AllocationHandle VertexBufferAllocHandle;
			DS::FreelistAllocator::AllocationHandle IndexBufferAllocHandle;
			DS::FreelistAllocator::AllocationHandle MeshEntryAllocHandle;
		};

		struct TextureEntry
		{
			D3D12::GPUTexture Texture;
			D3D12::ShaderResourceView SRV;
		};

		struct MaterialEntry
		{
			DS::FreelistAllocator::AllocationHandle MaterialAllocHandle;
		};

		struct VertexData
		{
			DXM::Vector3 position;
			DXM::Vector2 uv;
			DXM::Vector3 normal;
			DXM::Vector3 tangent;
		};

		struct MeshData
		{
			using IndexType = uint32_t;

			std::string Name;
			std::vector<VertexData> Vertices;
			std::vector<IndexType> Indices;
			float BoundingSphereRadius;

			bool LoadFromFile(const std::string& FilePath);
		};

		struct TextureData
		{
			std::vector<uint8_t> m_Data;
			DXM::Vector2 m_Dimensions;
			DXGI_FORMAT m_Format;

			bool LoadFromFile(const std::string& FilePath, DXGI_FORMAT Format);
		};

		struct MaterialData
		{
			struct MaterialRenderData
			{
				DXM::Vector3 m_Color = DXM::Vector3(1.f, 1.f, 1.f);
				int32_t m_AlbedoDescriptorIndex = -1;
				int32_t m_NormalMapDescriptorIndex = -1;
			};

			DXM::Vector3 m_Color = DXM::Vector3(1.f, 1.f, 1.f);
			std::shared_ptr<RenderAsset<TextureData, TextureEntry>> m_AlbedoTexture;
			std::shared_ptr<RenderAsset<TextureData, TextureEntry>> m_NormalMap;

			bool LoadFromFile(const std::string& FilePath)
			{
				return true;
			}
		};
	}
}