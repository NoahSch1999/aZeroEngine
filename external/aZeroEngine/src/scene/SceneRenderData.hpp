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
				StaticMesh(const ECS::TransformComponent& transform, const ECS::StaticMeshComponent& staticMesh, const DXM::Matrix& globalTransform)
				{
					const Asset::Mesh* mesh = staticMesh.GetMesh();
					const Asset::Material* material = staticMesh.GetMaterial();

					if (mesh && material
						&& mesh->GetRenderID() != std::numeric_limits<Asset::RenderID>::max()
						&& material->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
					{
						Transform = globalTransform;
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
				DXM::Matrix m_View;
				DXM::Matrix m_Projection;
				D3D12_VIEWPORT m_Viewport;
				D3D12_RECT m_ScizzorRect;

				// TODO: Rework into renderpass nicely
				bool m_ClearRenderTarget = true;
				bool m_ClearDepthTarget = true;
				bool m_ClearStencilTarget = true;
				uint32_t m_Layer = 0;
				std::optional<Rendering::RenderTarget*> m_RenderTarget;
				std::optional<Rendering::DepthStencilTarget*> m_DepthStencilTarget;

				struct GPUVersion
				{
					DXM::Matrix View;
					DXM::Matrix Projection;
					DirectX::BoundingFrustum Frustrum;
					GPUVersion(const Camera& camera)
						:View(camera.m_View), Projection(camera.m_Projection), Frustrum(camera.m_Frustrum) { }
				};

				Camera() = default;
				Camera(const ECS::CameraComponent& camera, const DXM::Matrix& globalTransform) {
					const DXM::Vector3 cameraPosition = DXM::Vector3::Transform(camera.m_Position, globalTransform);
					m_Frustrum = camera.m_OverridingFrustum.has_value() ? camera.m_OverridingFrustum.value() : DirectX::BoundingFrustum(camera.GetProjectionMatrix(), true);
					m_View = camera.GetViewMatrix();
					m_Projection = camera.GetProjectionMatrix();
					m_Viewport = camera.GetViewport();
					m_ScizzorRect = camera.GetScizzorRect();
					m_ClearRenderTarget = camera.m_ClearRenderTarget;
					m_ClearDepthTarget = camera.m_ClearDepthTarget;
					m_ClearStencilTarget = camera.m_ClearStencilTarget;
					m_RenderTarget = camera.m_RenderTarget;
					m_DepthStencilTarget = camera.m_DepthStencilTarget;
					m_Layer = camera.m_Layer;
				}

				GPUVersion CreateGPUVersion() const { return GPUVersion(*this); }
			};

			struct DirectionalLight
			{
				DXM::Vector3 m_Direction;
				DXM::Vector3 m_Color;
				float m_Intensity;

				DirectionalLight() = default;
				DirectionalLight(const ECS::DirectionalLightComponent& light)
					: m_Direction(light.Direction), m_Color(light.Color), m_Intensity(light.Intensity) {}
			};

			struct PointLight
			{
				// TODO: Fix so it gets this from the component
				DXM::Vector3 m_Position;
				DXM::Vector3 m_Color;
				float m_Intensity;
				float m_Attenuation;
				float m_BoundsRadius;

				PointLight() = default;
				PointLight(const ECS::PointLightComponent& light, const DXM::Matrix& globalTransform) {}
			};

			struct SpotLight
			{
				// TODO: Fix so it gets this from the component
				DXM::Vector3 m_Position;
				DXM::Vector3 m_Direction;
				DXM::Vector3 m_Color;
				float m_Intensity;
				float m_BoundsRadius;
				float m_ConeAngle;

				SpotLight() = default;
				SpotLight(const ECS::SpotLightComponent& light, const DXM::Matrix& globalTransform) {}
			};
		};
	}
}