#pragma once
#include <flecs.h>
#include "render_api/D3D12Include.hpp"
#include "physics/TriggerBody.hpp"
#include "assets/Mesh.hpp"
#include "assets/Material.hpp"
#include "render_api/resource/texture/DepthStencilTarget.hpp"
#include "render_api/resource/texture/RenderTarget.hpp"
#include <array>
#include <numbers>

namespace aZero
{
    namespace Component
    {
        struct Position
        {
            float x, y, z;

            Position() = default;

            Position(float x, float y, float z)
                : x(x), y(y), z(z)
            {}

            Position(const DXM::Vector3& v)
                : x(v.x), y(v.y), z(v.z)
            {}

            operator DXM::Vector3() const
            {
                return DXM::Vector3(x, y, z);
            }

            Position& operator+=(const DXM::Vector3& other)
            {
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
            }

            Position& operator-=(const DXM::Vector3& other)
            {
                x -= other.x;
                y -= other.y;
                z -= other.z;
                return *this;
            }
        };

        struct Rotation
        {
            float x, y, z;

            Rotation() = default;

            Rotation(float x, float y, float z)
                : x(x), y(y), z(z)
            {}

            Rotation(const DXM::Vector3& v)
                : x(v.x), y(v.y), z(v.z)
            {}

            Rotation& operator+=(const DXM::Vector3& other)
            {
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
            }

            Rotation& operator-=(const DXM::Vector3& other)
            {
                x -= other.x;
                y -= other.y;
                z -= other.z;
                return *this;
            }

            operator DXM::Vector3() const
            {
                return DXM::Vector3(x, y, z);
            }
        };

        struct Scale
        {
            float x, y, z;

            Scale() = default;

            Scale(float x, float y, float z)
                : x(x), y(y), z(z)
            {}

            Scale(const DXM::Vector3& v)
                : x(v.x), y(v.y), z(v.z)
            {}

            Scale& operator+=(const DXM::Vector3& other)
            {
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
            }

            Scale& operator-=(const DXM::Vector3& other)
            {
                x -= other.x;
                y -= other.y;
                z -= other.z;
                return *this;
            }

            operator DXM::Vector3() const
            {
                return DXM::Vector3(x, y, z);
            }
        };

        class Mesh {
        public:
            static constexpr uint32_t s_MaxNumberOfSubmeshes = 10;

            Mesh() = default;

            Mesh(const Asset::Mesh& mesh, const Asset::Material& material) {
                this->SetMesh(mesh, material);
            }

            // todo Implement a way to init with multiple materials

            void SetMesh(const Asset::Mesh& mesh, const Asset::Material& material) {
                if (mesh.GetRenderRef().IsValid() && material.GetRenderRef().IsValid()) {
                    m_MeshID = mesh.GetRenderRef().m_MeshletGlobalOffset; // todo Change so this becomes steady

                    const auto& [count, submeshes] = mesh.GetSubmeshes();
                    for (uint32_t i = 0; i < count; i++)
                    {
                        Submesh newSubmesh;
                        newSubmesh.MeshletCount = submeshes[i].MeshletCount;
                        newSubmesh.MeshletGlobalOffset = mesh.GetRenderRef().m_MeshletGlobalOffset + submeshes[i].MeshletOffset;
                        newSubmesh.VertexGlobalOffset = mesh.GetRenderRef().m_VertexGlobalOffset;
                        newSubmesh.m_Bounds = submeshes[i].Bounds;
                        newSubmesh.m_MaterialID = material.GetRenderRef().MaterialIndex;
                        m_Submeshes[i] = newSubmesh;
                    }
                    m_NumSubmeshes = count;
                }
            }

            void SetMaterial(uint32_t submeshIndex, const Asset::Material& material) {
                if (material.GetRenderRef().IsValid() && submeshIndex < m_NumSubmeshes) {
                    m_Submeshes[submeshIndex].m_MaterialID = material.GetRenderRef().MaterialIndex;
                }
            }

            struct Submesh
            {
                DirectX::BoundingSphere m_Bounds;
                uint32_t MeshletGlobalOffset, VertexGlobalOffset, MeshletCount;
                uint32_t m_MaterialID;
            };

            std::array<Submesh, s_MaxNumberOfSubmeshes> m_Submeshes;
            uint32_t m_NumSubmeshes = 0;
            uint32_t m_MeshID;

        private:
        };

        struct PointLight {
            DXM::Vector3 color = { 0.f,0.f,0.f };
            float intensity = 1.f;
            float falloffStart = 0.f;
            float falloffEnd = 1.f;
        };

        struct SpotLight {
            DXM::Vector3 color = { 0.f,0.f,0.f };
            float intensity = 1.f;
            float falloffStart = 0.f;
            float falloffEnd = 1.f;
            float spotPower = 1.f;
        };

        struct DirectionalLight {
            DXM::Vector3 color = { 0.f,0.f,0.f };
            float intensity = 1.f;
        };

        // todo Make it support multiple colliders in one component
        struct Triggerbody : public Physics::TriggerBody
        {
            Triggerbody() = default;

            Triggerbody(const JPH::BodyCreationSettings& settings)
                :Physics::TriggerBody(settings) 
            {
                m_BodySettings.mIsSensor = true;
            }

            Triggerbody& operator=(const JPH::BodyCreationSettings& settings)
            {
                Physics::TriggerBody::operator=(settings);
                m_BodySettings.mIsSensor = true;
                return *this;
            }
        };

        struct Rigidbody : public Physics::TriggerBody
        {
            Rigidbody() = default;

            Rigidbody(const JPH::BodyCreationSettings& settings)
                :Physics::TriggerBody(settings) { }

            Rigidbody& operator=(const JPH::BodyCreationSettings& settings)
            {
                Physics::TriggerBody::operator=(settings);
                return *this;
            }
        };

        struct Camera {
            enum class EProjectionType { Ortographic, Perspective };

            float Fov = std::numbers::pi_v<float> / 2.f;
            float Near = 0.001f;
            float Far = 1000.f;
            Helper::Rectangle Viewport = { 0,0,0,0 };
            int32_t Layer = 0;
            EProjectionType ProjectionType;
            bool Active = true;

            Rendering::RenderTarget* Rtv = nullptr;
            Rendering::DepthStencilTarget* Dsv = nullptr;

            struct BoundingFrustum
            {
                DXM::Vector4 Rotation;

                DXM::Vector3 Position;
                float RightSlope;

                float LeftSlope;
                float TopSlope;
                float BottomSlope;
                float Near;

                float Far;
            };

            BoundingFrustum GetFrustum() const
            {
                DirectX::BoundingFrustum frustumTemp = DirectX::BoundingFrustum(this->GetProjectionMatrix(), true);
                BoundingFrustum frustum;
                frustum.Rotation = frustumTemp.Orientation;
                frustum.Position = frustumTemp.Origin;
                frustum.RightSlope = frustumTemp.RightSlope;
                frustum.LeftSlope = frustumTemp.LeftSlope;
                frustum.TopSlope = frustumTemp.TopSlope;
                frustum.BottomSlope = frustumTemp.BottomSlope;
                frustum.Near = frustumTemp.Near;
                frustum.Far = frustumTemp.Far;
                return frustum;
            }

            Camera() = default;
            Camera(EProjectionType projectionType, float fov, float nearPlane,
                float farPlane, bool isActive, const Helper::Rectangle viewport, Rendering::RenderTarget* rtv = nullptr, Rendering::DepthStencilTarget* dsv = nullptr)
                :ProjectionType(projectionType), Fov(fov), Near(nearPlane), Far(farPlane),
                Active(isActive), Viewport(viewport), Rtv(rtv), Dsv(dsv)
            {
               
            }

            Camera(float fov, float nearPlane,
                float farPlane, bool isActive, const Helper::Rectangle viewport, Rendering::RenderTarget* rtv = nullptr, Rendering::DepthStencilTarget* dsv = nullptr)
                :Fov(fov), Near(nearPlane), Far(farPlane),
                Active(isActive), Viewport(viewport), Rtv(rtv), Dsv(dsv)
            {

            }

            D3D12_VIEWPORT GetViewport() const
            {
                D3D12_VIEWPORT viewport;
                viewport.TopLeftX = Viewport.TopX;
                viewport.TopLeftY = Viewport.TopY;
                viewport.Width = Viewport.Width;
                viewport.Height = Viewport.Height;
                viewport.MinDepth = 0;
                viewport.MaxDepth = 1;
                return viewport;
            }

            D3D12_RECT GetScizzorRect() const
            {
                D3D12_RECT ScizzorRect;
                ScizzorRect.left = static_cast<LONG>(Viewport.TopX);
                ScizzorRect.top = static_cast<LONG>(Viewport.TopY);
                ScizzorRect.right = static_cast<LONG>(Viewport.TopX + Viewport.Width);
                ScizzorRect.bottom = static_cast<LONG>(Viewport.TopY + Viewport.Height);
                return ScizzorRect;
            }

            // This isn't really supposed to be a member function but since we store the position and rotation away from the camera this is still a nice function to have...
            DXM::Matrix GetViewMatrix(const Component::Position& position,
                const Component::Rotation& rotationRadians) const
            {
                DXM::Matrix rotationMatrix =
                    DXM::Matrix::CreateFromYawPitchRoll(
                        rotationRadians.x,
                        rotationRadians.y,
                        rotationRadians.z
                    );

                DXM::Vector3 forward = DXM::Vector3::TransformNormal(
                    DXM::Vector3(0, 0, 1),
                    rotationMatrix
                );

                DXM::Vector3 up = DXM::Vector3::TransformNormal(
                    DXM::Vector3(0, 1, 0),
                    rotationMatrix
                );

                return DXM::Matrix::CreateLookAt(
                    position,
                    position + forward,
                    up
                );
            }
            //

            DXM::Matrix GetProjectionMatrix() const
            {
                if (ProjectionType == EProjectionType::Perspective)
                {
                    return DXM::Matrix::CreatePerspectiveFieldOfView(Fov, Viewport.Width / Viewport.Height, Near, Far);
                }
                else
                {
                    return DXM::Matrix::CreateOrthographic(Viewport.Width, Viewport.Height, Near, Far);
                }
            }

            DXM::Matrix GetViewProjectionMatrix(const Component::Position& cameraPosition, const Component::Rotation& cameraRotation) const
            {
                return this->GetViewMatrix(cameraPosition, cameraRotation) * this->GetProjectionMatrix();
            }
        };

        struct Static { };
    }
}