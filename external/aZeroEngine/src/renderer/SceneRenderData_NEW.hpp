#pragma once
#include "ecs/Components.hpp"
#include "misc/HelperFunctions.hpp"

namespace aZero::Rendering {
	namespace GPUProxy {
		using MeshMaterialID = uint32_t;
		constexpr static MeshMaterialID InvalidMeshMaterialID = std::numeric_limits<MeshMaterialID>::max();

		struct StaticMeshInstance {

			DXM::Matrix m_WorldTransform;
			uint32_t m_MeshletBuffer_Bindless;
			uint32_t m_MeshletCount;
			uint32_t m_MaterialIndex;
			DirectX::BoundingSphere m_MeshBounds;

			StaticMeshInstance() = default;
			StaticMeshInstance(const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
			{
				// TODO: Avoid this much data
				m_WorldTransform = /*DXM::Matrix::CreateScale(scale.vec), */DXM::Matrix::CreateFromYawPitchRoll(rotation) * DXM::Matrix::CreateTranslation(position);
				m_MeshletBuffer_Bindless = mesh.GetMeshID();
				m_MeshletCount = mesh.GetMeshletCount();
				m_MaterialIndex = mesh.GetMaterialID();
				m_MeshBounds = mesh.GetBounds();
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
			DirectX::BoundingFrustum m_Frustrum;
			RSInfo m_RSInfo;

			Camera() = default;
			Camera(const Component::Camera& camera, const Component::Position& position, const Component::Rotation& rotation)
			{
				// TODO: Populate members correctly
				m_View = camera.GetViewMatrix(position, rotation);
				m_Projection = camera.GetProjectionMatrix();
				m_Frustrum = DirectX::BoundingFrustum(m_Projection, true);
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