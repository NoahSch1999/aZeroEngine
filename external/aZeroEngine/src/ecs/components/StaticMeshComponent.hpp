#pragma once

#include "assets/Mesh.hpp"
#include "assets/Material.hpp"

namespace aZero
{
	namespace ECS
	{
		class StaticMeshComponent
		{
		private:
			Asset::Mesh* m_MeshReference = nullptr;
			Asset::Material* m_MaterialReference = nullptr;

		public:
			StaticMeshComponent() = default;
			StaticMeshComponent(Asset::Mesh* mesh, Asset::Material* material)
				:m_MeshReference(mesh), m_MaterialReference(material){ }
			
			void SetMesh(Asset::Mesh* mesh) { m_MeshReference = mesh; }
			const Asset::Mesh* GetMesh() const { return m_MeshReference; }

			void SetMaterial(Asset::Material* material) { m_MaterialReference = material; }
			const Asset::Material* GetMaterial() const { return m_MaterialReference; }
		};
	}
}