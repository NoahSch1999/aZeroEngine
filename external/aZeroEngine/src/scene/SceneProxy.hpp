#pragma once
#include "misc/SparseMappedVector.hpp"
#include "SceneRenderData.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneProxy
		{
		public:
			SceneProxy() = default;

			void UpdateStaticMesh(ECS::EntityID id, const ECS::TransformComponent* transformComponent, const ECS::NewStaticMeshComponent* staticMeshComponent);
			void UpdateCamera(ECS::EntityID id, const ECS::CameraComponent* cameraComponent);
			void UpdateDirectionalLight(ECS::EntityID id, const ECS::DirectionalLightComponent* lightComponent);
			void UpdatePointLight(ECS::EntityID id, const ECS::PointLightComponent* lightComponent);
			void UpdateSpotLight(ECS::EntityID id, const ECS::SpotLightComponent* lightComponent);

		private:
			/*
			NOTE:
				If we move to a fully gpu-driven rendering pipeline this data need to have a mirrored version in vram.
				We can probably skip having mirrored versions of the cameras since we probably can upload them per-frame.
				In best case we can simply change the logic of adding a new entry in the SparseMappedVector to include gpu-upload of the data too.
					Then, instead of the renderer owning the instance buffers etc, we can have the renderer fetch the buffers from the scene proxy and set them.
			*/
			DataStructures::SparseMappedVector<ECS::EntityID, RenderData::StaticMesh> m_StaticMeshes;
			DataStructures::SparseMappedVector<ECS::EntityID, RenderData::Camera> m_Cameras;
			DataStructures::SparseMappedVector<ECS::EntityID, RenderData::DirectionalLight> m_DirectionalLights;
			DataStructures::SparseMappedVector<ECS::EntityID, RenderData::PointLight> m_PointLights;
			DataStructures::SparseMappedVector<ECS::EntityID, RenderData::SpotLight> m_SpotLights;
		};
	}
}