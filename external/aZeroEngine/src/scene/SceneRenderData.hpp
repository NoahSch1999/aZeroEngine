#pragma once
#include "ecs/aZeroECS.hpp"

namespace aZero
{
	namespace Scene
	{
		struct RenderData
		{
			struct StaticMesh
			{
				DXM::Matrix m_Transform = DXM::Matrix::Identity;
				Asset::RenderID m_MeshIndex = Asset::InvalidRenderID;
				Asset::RenderID  m_MaterialIndex = Asset::InvalidRenderID;
				DirectX::BoundingSphere m_BoundingSphere;
				uint32_t m_NumVertices = 0;

				StaticMesh() = default;
				StaticMesh(const ECS::TransformComponent& transform, const ECS::NewStaticMeshComponent& staticMesh)
				{
					const Asset::NewMesh* mesh = staticMesh.GetMesh();
					const Asset::NewMaterial* material = staticMesh.GetMaterial();

					if (mesh && material
						&& mesh->GetRenderID() != std::numeric_limits<Asset::RenderID>::max()
						&& material->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
					{
						m_MeshIndex = mesh->GetRenderID();
						m_MaterialIndex = material->GetRenderID();
						m_Transform = transform.GetTransform();
						m_NumVertices = mesh->m_VertexData.Positions.size();

						// TODO: Change this
						const float XAxisScale = m_Transform.m[0][0];
						const float YAxisScale = m_Transform.m[1][1];
						const float ZAxisScale = m_Transform.m[2][2];
						const float MaxScaleAxis = std::max(abs(XAxisScale), std::max(abs(YAxisScale), abs(ZAxisScale)));
						m_BoundingSphere = DirectX::BoundingSphere(
							{ m_Transform.Translation().x, m_Transform.Translation().y, m_Transform.Translation().z },
							mesh->m_BoundingSphereRadius * MaxScaleAxis
						);
						//
					}
				}

				bool IsRenderReady() const
				{
					return m_MeshIndex != Asset::InvalidRenderID && m_MaterialIndex != Asset::InvalidRenderID;
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