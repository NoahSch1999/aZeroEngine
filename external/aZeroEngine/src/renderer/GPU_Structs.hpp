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
        float Attenuation;
        float BoundsRadius;

        PointLight() = default;
        PointLight(const Component::PointLight& other, const Component::Position& position)
        {
            Position = position;
            Color = other.color;
            Intensity = other.intensity;
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
        float BoundsRadius;
        float ConeAngle;

        SpotLight() = default;
        SpotLight(const Component::SpotLight& other, const Component::Position& position, const Component::Rotation& rotation)
        {
            Position = position;
            Rotation = rotation;
            Color = other.color;
            Intensity = other.intensity;
            ConeAngle = other.coneAngle;
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
            Rotation = rotation;
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