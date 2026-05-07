#pragma once
#include "ecs/Components.hpp"
#include "misc/HelperFunctions.hpp"

namespace aZero::Rendering {
	namespace GPUProxy {
		using MeshMaterialID = uint32_t;
		constexpr static MeshMaterialID InvalidMeshMaterialID = std::numeric_limits<MeshMaterialID>::max();

		struct StaticMesh {
			DXM::Matrix m_Transform;
			MeshMaterialID m_MeshMaterialID;

			StaticMesh() = default;
			StaticMesh(const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale)
			{
				// TODO: Populate members correctly
				m_Transform = /*DXM::Matrix::CreateScale(scale.vec), */DXM::Matrix::CreateFromYawPitchRoll(rotation) * DXM::Matrix::CreateTranslation(position);
				m_MeshMaterialID = Helper::Pack16To32(mesh.GetMeshID(), mesh.GetMaterialID());
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
			DXM::Matrix m_View;
			DXM::Matrix m_Projection;
			DirectX::BoundingFrustum m_Frustrum;

			struct RasterInfo {
				D3D12_VIEWPORT Viewport;
				D3D12_RECT ScizzorRect;

				RasterInfo() = default;
				RasterInfo(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scizzorRect)
					:Viewport(viewport), ScizzorRect(scizzorRect){}
			};

			Camera() = default;
			Camera(const Component::Camera& camera, const Component::Position& position, const Component::Rotation& rotation)
			{
				// TODO: Populate members correctly
				m_View = camera.GetViewMatrix(position, rotation);
				m_Projection = camera.GetProjectionMatrix();
				m_Frustrum = DirectX::BoundingFrustum(m_Projection, true);
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