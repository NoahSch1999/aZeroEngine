#pragma once

#include "assets/Mesh.hpp"
#include "assets/Material.hpp"

namespace aZero
{
	namespace ECS
	{
		class StaticMeshComponent
		{
		public:
			Asset::AssetHandle<Asset::Mesh> m_MeshReference;
			Asset::AssetHandle<Asset::Material> m_MaterialReference;

			StaticMeshComponent() = default;
		};
	}
}