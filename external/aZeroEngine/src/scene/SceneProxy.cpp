#include "SceneProxy.hpp"

namespace aZero
{
	namespace Scene
	{
		void SceneProxy::UpdateStaticMesh(ECS::EntityID id, const ECS::TransformComponent* transformComponent, const ECS::StaticMeshComponent* staticMeshComponent)
		{
			if (!transformComponent || !staticMeshComponent) {
				m_StaticMeshes.Remove(id);
				return;
			}

			RenderData::StaticMesh staticMesh(*transformComponent, *staticMeshComponent);
			if (staticMesh.IsRenderReady()) {

				m_StaticMeshes.AddOrUpdate(id, std::move(staticMesh));
			}
			else {
				m_StaticMeshes.Remove(id);
			}
		}

		void SceneProxy::UpdateCamera(ECS::EntityID id, const ECS::CameraComponent* cameraComponent)
		{
			if (!cameraComponent) {
				m_Cameras.Remove(id);
				return;
			}

			if (cameraComponent->m_IsActive) {
				m_Cameras.AddOrUpdate(id, RenderData::Camera(*cameraComponent));
			}
			else {
				m_Cameras.Remove(id);
			}
		}

		void SceneProxy::UpdateDirectionalLight(ECS::EntityID id, const ECS::DirectionalLightComponent* lightComponent)
		{
			if (!lightComponent) {
				m_DirectionalLights.Remove(id);
				return;
			}

			m_DirectionalLights.AddOrUpdate(id, RenderData::DirectionalLight(*lightComponent));
		}

		void SceneProxy::UpdatePointLight(ECS::EntityID id, const ECS::PointLightComponent* lightComponent)
		{
			if (!lightComponent) {
				m_PointLights.Remove(id);
				return;
			}

			m_PointLights.AddOrUpdate(id, RenderData::PointLight(*lightComponent));
		}

		void SceneProxy::UpdateSpotLight(ECS::EntityID id, const ECS::SpotLightComponent* lightComponent)
		{
			if (!lightComponent) {
				m_SpotLights.Remove(id);
				return;
			}

			m_SpotLights.AddOrUpdate(id, RenderData::SpotLight(*lightComponent));
		}
	}
}