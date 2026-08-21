#pragma once
#include "render_api/D3D12Include.hpp"
#include "ecs/Components.hpp"

namespace aZero::Rendering::GPU_Struct
{
    struct PointLight
    {
    public:
        DXM::Vector3 Position;
        DXM::Vector3 Color;
        float Intensity;
        float FalloffStart;
        float FalloffEnd;

        PointLight() = default;
        PointLight(const Component::PointLight& other, const Component::Position& position)
        {
            Position = position;
            Color = other.color;
            Intensity = other.intensity;
            FalloffStart = other.falloffStart;
            FalloffEnd = other.falloffEnd;
            // todo Init rest
        }
    };

    struct SpotLight
    {
    public:
        DXM::Vector3 Position;
        DXM::Vector3 Rotation;
        DXM::Vector3 Color;
        float Intensity;
        float FalloffStart;
        float FalloffEnd;

        float SpotPower;

        SpotLight() = default;
        SpotLight(const Component::SpotLight& other, const Component::Position& position, const Component::Rotation& rotation)
        {
            DXM::Matrix rotationMatrix =
                DXM::Matrix::CreateFromYawPitchRoll(
                    rotation.x,
                    rotation.y,
                    rotation.z
                );

            Rotation = DXM::Vector3::TransformNormal(
                DXM::Vector3(0, 0, 1),
                rotationMatrix
            );
            Rotation.Normalize();

            Position = position;
            Color = other.color;
            Intensity = other.intensity;
            FalloffStart = other.falloffStart;
            FalloffEnd = other.falloffEnd;
            SpotPower = other.spotPower;
            // todo Init rest
        }
    };

    struct DirectionalLight
    {
    public:
        DXM::Vector3 Rotation;
        DXM::Vector3 Color;
        float Intensity;

        DirectionalLight() = default;
        DirectionalLight(const Component::DirectionalLight& other, const Component::Rotation& rotation)
        {
            DXM::Matrix rotationMatrix =
                DXM::Matrix::CreateFromYawPitchRoll(
                    rotation.x,
                    rotation.y,
                    rotation.z
                );

            Rotation = DXM::Vector3::TransformNormal(
                DXM::Vector3(0, 0, 1),
                rotationMatrix
            );
            Rotation.Normalize();

            Color = other.color;
            Intensity = other.intensity;
        }
    };

    struct CameraData
    {
        DXM::Matrix ViewMatrix;
        DXM::Matrix ViewProjectionMatrix;
        Component::Camera::BoundingFrustum Frustum;
    };

    struct PixelConstants
    {
        uint32_t NumPointLights;
        uint32_t NumSpotLights;
        uint32_t NumDirectionalLights;
        DXM::Vector3 CameraDirection;
        DXM::Vector3 CameraPosition;
    };

    struct IndirectArgumentCounter
    {
        uint32_t Count;
    };

    struct InstanceData
    {
        DXM::Matrix Transform;
    };

    struct IndirectArgumentConstantData
    {
        DXM::Matrix WorldMatrix;
        uint32_t GlobalMeshletOffset;
        uint32_t GlobalVertexOffset;
        uint32_t MaterialIndex;
        uint32_t MeshletCount;
    };

    // TODO: Replace GlobalMeshletOffset, GlobalVertexOffset, MeshletCount with a LOD-info index that is used in the object-cull shader to find the LOD that contains them
    struct ObjectCullData
    {
        DirectX::BoundingSphere Bounds;
        uint32_t InstanceDataIndex;
        uint32_t GlobalMeshletOffset;
        uint32_t GlobalVertexOffset;
        uint32_t MeshletCount;
        uint32_t MaterialIndex;
    };

    struct MeshCullConstantsData
    {
        uint32_t MeshInstanceCount;
        uint32_t Pad[3];
    };

    struct IndirectArguments
    {
        IndirectArgumentConstantData Data;
        uint32_t GroupX;
        uint32_t GroupY;
        uint32_t GroupZ;
    };

    struct PhongPixelConstantData
    {
        uint32_t SamplerIndex;
        uint32_t MaterialBuffer;
        uint32_t PointLightBuffer;
        uint32_t SpotLightBuffer;
        uint32_t DirectionalLightBuffer;
        float Time;
    };
}