#pragma once
#include "ecs/Components.hpp"
#include "misc/HelperFunctions.hpp"

namespace aZero::Rendering {
	namespace GPUProxy {
		using MeshMaterialID = uint32_t;
		constexpr static MeshMaterialID InvalidMeshMaterialID = std::numeric_limits<MeshMaterialID>::max();

		struct StaticMeshInstance {

			DXM::Matrix m_WorldTransform;
			uint32_t m_MeshBufferIndex;
			uint32_t m_MeshletOffset;
			uint32_t m_MeshletCount;
			uint32_t m_PrimitiveOffset, m_VertexOffset, m_IndexOffset;
			uint32_t m_MaterialIndex;
			DirectX::BoundingSphere m_MeshBounds;

			StaticMeshInstance() = default;

			StaticMeshInstance(Asset::RenderID meshID, const Component::Mesh::Submesh& submesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
			{
				// TODO: Avoid this much data
				m_WorldTransform = DXM::Matrix::CreateScale(scale) * DXM::Matrix::CreateFromYawPitchRoll(rotation) * DXM::Matrix::CreateTranslation(position);
				m_MeshBufferIndex = meshID;
				m_MeshletCount = submesh.MeshletCount;
				m_MeshletOffset = submesh.MeshletOffset;
				m_PrimitiveOffset = submesh.PrimitiveOffset;
				m_VertexOffset = submesh.VertexOffset;
				m_IndexOffset = submesh.IndexOffset;
				m_MaterialIndex = submesh.m_MaterialID;
				DirectX::BoundingSphere bounds;
				submesh.m_Bounds.Transform(bounds, m_WorldTransform);
				m_MeshBounds = bounds;
			}
		};

		struct PointLight {
			DXM::Vector3 m_Position;
			DXM::Vector3 m_Color;
			float m_Radius;
			float m_Intensity;

			PointLight() = default;
			PointLight(const Component::PointLight& pointLight, const Component::Position& position)
			{
				// TODO: Populate members
			}
		};

		struct Camera {
			struct RSInfo {
				D3D12_VIEWPORT Viewport;
				D3D12_RECT ScizzorRect;

				RSInfo() = default;
				RSInfo(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scizzorRect)
					:Viewport(viewport), ScizzorRect(scizzorRect) {}
			};

			DXM::Matrix m_View;
			DXM::Matrix m_Projection;
			Component::Camera::BoundingFrustum m_Frustrum;
			RSInfo m_RSInfo;

			Camera() = default;
			Camera(const Component::Camera& camera, const Component::Position& position, const Component::Rotation& rotation)
			{
				// TODO: Populate members correctly
				m_View = camera.GetViewMatrix(position, rotation);
				m_Projection = camera.GetProjectionMatrix();
				m_Frustrum = camera.GetFrustum();
				m_RSInfo = RSInfo(camera.GetViewport(), camera.GetScizzorRect());
			}
		};

		/*
		* TODO: Other lights
		struct SpotLight {
		DXM::Vector3 color;
		DXM::Vector3 direction;
		float coneAngle;
		float intensity;
		};

		struct DirectionalLight {
		DXM::Vector3 color;
		DXM::Vector3 direction;
		float intensity;
		};
		*/
	}
}