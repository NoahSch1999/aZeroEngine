#pragma once
#include "ecs/FlecsInclude.hpp"
#include "graphics_api/D3D12Include.hpp"
#include "assets/Mesh.hpp"
#include "assets/Texture.hpp"
#include "assets/Material.hpp"
#include "physics/TriggerBody.hpp"
#include <array>

namespace aZero
{
    // TODO: Make components user friendly
    namespace Component
    {
        // Why doesn't doing this work with flecs? The cached queries seem to break...
        /*
        struct Position : public DXM::Vector3 {}
        */
        
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

            operator DXM::Vector3() const
            {
                return DXM::Vector3(x, y, z);
            }
        };

        class Mesh {
        public:
            Mesh() = default;
            Mesh(const Asset::Mesh& mesh, const Asset::Material& material) {
                this->SetMesh(mesh, material);
            }

            void SetMesh(const Asset::Mesh& mesh, const Asset::Material& material) {
                if (mesh.GetRenderID() != Asset::InvalidRenderID && material.GetRenderID() != Asset::InvalidRenderID) {
                    m_MeshID = mesh.GetRenderID();

                    auto& submeshes = mesh.GetSubmeshes();
                    for (uint32_t i = 0; i < submeshes.size(); i++)
                    {
                        Submesh newSubmesh;
                        newSubmesh.MeshletCount = submeshes[i].MeshletCount;
                        newSubmesh.MeshletOffset = submeshes[i].MeshletOffset;
                        newSubmesh.m_Bounds = submeshes[i].Bounds;
                        newSubmesh.m_MaterialID = material.GetRenderID();
                        m_Submeshes[i] = newSubmesh;
                    }
                    m_NumSubmeshes = submeshes.size();
                }
            }

            void SetMaterial(uint32_t submeshIndex, const Asset::Material& material) {
                if (material.GetRenderID() != Asset::InvalidRenderID && submeshIndex < m_NumSubmeshes) {
                    m_Submeshes[submeshIndex].m_MaterialID = material.GetRenderID();
                }
            }

            /*void SetMesh(Asset::RenderID renderID) {
                if (renderID != Asset::InvalidRenderID) {
                    m_MeshID = renderID;
                }
            }

            void SetMaterial(Asset::RenderID renderID) {
                if (renderID != Asset::InvalidRenderID) {
                    m_MaterialID = renderID;
                }
            }*/

            Asset::RenderID GetMeshID() const { return m_MeshID; }

            struct Submesh
            {
                DirectX::BoundingSphere m_Bounds;
                uint32_t MeshletOffset, MeshletCount;
                Asset::RenderID m_MaterialID;
            };

            std::array<Submesh, 10> m_Submeshes;
            uint32_t m_NumSubmeshes = 0;
            Asset::RenderID m_MeshID;

        private:
        };

        struct PointLight {
            DXM::Vector3 color;
            float radius;
            float intensity;
        };

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

        struct Rigidbody : public Physics::TriggerBody
        {

        };

        struct Camera {
            float fov;
            float nearPlane;
            float farPlane;
            bool isActive = true;
            DXM::Vector2 topleft;
            DXM::Vector2 dimensions;

            struct alignas(16) BoundingFrustum
            {
                DXM::Vector4 Rotation;

                DXM::Vector3 Position;
                float RightSlope;

                float LeftSlope;
                float TopSlope;
                float BottomSlope;
                float Near;

                float Far;
                DXM::Vector3 pad;
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
            Camera(float fov, float nearPlane,
                float farPlane, bool isActive, const DXM::Vector2 topleft, const DXM::Vector2& dimensions)
                :fov(fov), nearPlane(nearPlane), farPlane(farPlane),
                isActive(isActive), topleft(topleft), dimensions(dimensions)
            {
               
            }

            D3D12_VIEWPORT GetViewport() const
            {
                D3D12_VIEWPORT Viewport;
                Viewport.TopLeftX = topleft.x;
                Viewport.TopLeftY = topleft.y;
                Viewport.Width = dimensions.x;
                Viewport.Height = dimensions.y;
                Viewport.MinDepth = 0;
                Viewport.MaxDepth = 1;
                return Viewport;
            }

            D3D12_RECT GetScizzorRect() const
            {
                D3D12_RECT ScizzorRect;
                ScizzorRect.left = static_cast<LONG>(topleft.x);
                ScizzorRect.top = static_cast<LONG>(topleft.y);
                ScizzorRect.right = static_cast<LONG>(topleft.x + dimensions.x);
                ScizzorRect.bottom = static_cast<LONG>(topleft.y + dimensions.y);
                return ScizzorRect;
            }

            // This isn't really supposed to be a member function but since we store the position and rotation away from the camera this is still a nice function to have...
            DXM::Matrix GetViewMatrix(const Component::Position& cameraPosition, const Component::Rotation& cameraRotation) const
            {
                // TODO: Use rotation to orient the view
                return DXM::Matrix::CreateLookAt(cameraPosition, cameraPosition + DXM::Vector3(0, 0, 1), { 0,1,0 });
            }
            //

            DXM::Matrix GetProjectionMatrix() const
            {
                return DXM::Matrix::CreatePerspectiveFieldOfView(fov, dimensions.x / dimensions.y, nearPlane, farPlane);
            }

            DXM::Matrix GetViewProjectionMatrix(const Component::Position& cameraPosition, const Component::Rotation& cameraRotation) const
            {
                // TODO: Use rotation to orient the view
                return this->GetViewMatrix(cameraPosition, cameraRotation) * this->GetProjectionMatrix();
            }
        };
    }
}