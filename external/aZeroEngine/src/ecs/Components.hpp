#pragma once
#include "ecs/FlecsInclude.hpp"
#include "graphics_api/D3D12Include.hpp"
#include "assets/Mesh.hpp"
#include "assets/Texture.hpp"
#include "assets/Material.hpp"
#include "physics/TriggerBody.hpp"

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
                this->SetMesh(mesh);
                this->SetMaterial(material);
            }

            void SetMesh(const Asset::Mesh& mesh) {
                if (mesh.GetRenderID() != Asset::InvalidRenderID) {
                    m_MeshID = mesh.GetRenderID();
                    m_MeshletCount = mesh.GetVertexData().Meshlets.size();
                    m_MeshBounds = mesh.GetVertexData().Bounds;
                }
            }

            void SetMaterial(const Asset::Material& material) {
                if (material.GetRenderID() != Asset::InvalidRenderID) {
                    m_MaterialID = material.GetRenderID();
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
            DirectX::BoundingSphere GetBounds() const { return m_MeshBounds; }
            uint32_t GetMeshletCount() const { return m_MeshletCount; }
            Asset::RenderID GetMaterialID() const { return m_MaterialID; }

        private:
            DirectX::BoundingSphere m_MeshBounds;
            uint32_t m_MeshletCount;
            Asset::RenderID m_MeshID;
            Asset::RenderID m_MaterialID;
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

            DirectX::BoundingFrustum GetFrustum() const
            {
                return DirectX::BoundingFrustum(this->GetProjectionMatrix(), true);
            }
        };
    }
}