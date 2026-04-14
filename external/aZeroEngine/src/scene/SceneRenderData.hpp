#pragma once
#include "ecs/aZeroECS.hpp"
#include "misc/HelperFunctions.hpp"

namespace aZero
{
	namespace Scene
	{
		struct RenderData
		{
			using BatchID = uint32_t;
			constexpr static BatchID InvalidBatchID = std::numeric_limits<BatchID>::max();

			struct StaticMesh
			{
				DXM::Matrix Transform;
				uint32_t BatchID;

				StaticMesh() = default;
				StaticMesh(const ECS::TransformComponent& transform, const ECS::StaticMeshComponent& staticMesh)
				{
					const Asset::Mesh* mesh = staticMesh.GetMesh();
					const Asset::Material* material = staticMesh.GetMaterial();

					if (mesh && material
						&& mesh->GetRenderID() != std::numeric_limits<Asset::RenderID>::max()
						&& material->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
					{
						Transform = transform.GetTransform();
						BatchID = Helper::Pack16To32(staticMesh.GetMesh()->GetRenderID(), staticMesh.GetMaterial()->GetRenderID());
					}
				}

				bool IsRenderReady() const
				{
					return BatchID != InvalidBatchID;
				}
			};

			struct Camera
			{
				DirectX::BoundingFrustum m_Frustrum;
				DXM::Matrix m_ViewProjectionMatrix;
				D3D12_VIEWPORT m_Viewport;
				D3D12_RECT m_ScizzorRect;
				bool m_IsActive;

				Camera() = default;
				Camera(const ECS::CameraComponent& camera) {
					m_Frustrum = DirectX::BoundingFrustum(camera.GetProjectionMatrix(), true);
					m_ViewProjectionMatrix = camera.GetViewMatrix() * camera.GetProjectionMatrix();
					m_Viewport = camera.GetViewport();
					m_ScizzorRect = camera.GetScizzorRect();
				}
			};

			struct DirectionalLight
			{
				DXM::Vector3 m_Direction;
				DXM::Vector3 m_Color;
				float m_Intensity;

				DirectionalLight() = default;
				DirectionalLight(const ECS::DirectionalLightComponent& light)
					: m_Direction(light.Direction), m_Color(light.Color), m_Intensity(light.Intensity) {
				}
			};

			struct PointLight
			{
				DXM::Vector3 m_Color;
				float m_Intensity;
				DXM::Vector3 m_Position;
				float m_FalloffFactor;

				PointLight() = default;
				PointLight(const ECS::PointLightComponent& light)
					: m_Position(light.Position), m_Color(light.Color), m_Intensity(light.Intensity), m_FalloffFactor(light.FalloffFactor) {
				}
			};

			struct SpotLight
			{
				DXM::Vector3 m_Direction;
				DXM::Vector3 m_Position;
				DXM::Vector3 m_Color;
				float m_ConeRadius;
				float m_Range;
				float m_Intensity;

				SpotLight() = default;
				SpotLight(const ECS::SpotLightComponent& light)
					: m_Direction(light.Direction), m_Position(light.Position), m_Color(light.Color), m_Intensity(light.Intensity), m_ConeRadius(light.CutoffAngle), m_Range(light.Range) {
				}
			};
		};
	}
}