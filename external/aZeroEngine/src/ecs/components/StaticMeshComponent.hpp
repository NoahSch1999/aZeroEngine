#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "renderer/render_asset/RenderAsset.hpp"

namespace aZero
{
	namespace ECS
	{
		class StaticMeshComponent
		{
		public:
			std::shared_ptr<Asset::Mesh> m_MeshReference;
			std::shared_ptr<Asset::Material> m_MaterialReference;

			StaticMeshComponent() = default;
			
			StaticMeshComponent(const StaticMeshComponent& Other)
			{
				m_MeshReference = Other.m_MeshReference;
				m_MaterialReference = Other.m_MaterialReference;
			}

			StaticMeshComponent(StaticMeshComponent&& Other) noexcept
			{
				m_MeshReference = std::move(Other.m_MeshReference);
				m_MaterialReference = std::move(Other.m_MaterialReference);
			}

			StaticMeshComponent& operator=(StaticMeshComponent&& Other) noexcept
			{
				if (this != &Other)
				{
					m_MeshReference = std::move(Other.m_MeshReference);
					m_MaterialReference = std::move(Other.m_MaterialReference);
				}

				return *this;
			}

			StaticMeshComponent& operator=(const StaticMeshComponent& Other)
			{
				m_MeshReference = Other.m_MeshReference;
				m_MaterialReference = Other.m_MaterialReference;
				return *this;
			}

		};
	}
}