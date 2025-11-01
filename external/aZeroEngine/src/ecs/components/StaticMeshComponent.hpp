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
			std::weak_ptr<Asset::Mesh> m_MeshReference;
			std::weak_ptr<Asset::Material> m_MaterialReference;

			StaticMeshComponent() = default;
		};
	}
}