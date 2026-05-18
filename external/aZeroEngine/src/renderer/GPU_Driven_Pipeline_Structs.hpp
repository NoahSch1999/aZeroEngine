#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "ecs/Components.hpp"

namespace aZero::Rendering::GPU_Struct
{
    struct MeshInstance
    {
        DXM::Matrix WorldTransform;
        uint32_t MeshBufferIndex;
        uint32_t MeshletOffset;
        uint32_t MeshletCount;
        uint32_t MaterialIndex;
        DirectX::BoundingSphere MeshBounds;
    };

    struct IndirectArgumentCounter
    {
        uint32_t Count;
    };

    struct IndirectArgumentConstantData
    {
        uint32_t MeshInstanceIndex;
        uint32_t Pad1;
        uint32_t Pad2;
        uint32_t Pad3;
    };

    struct IndirectArguments
    {
        IndirectArgumentConstantData Data;
        uint32_t GroupX;
        uint32_t GroupY;
        uint32_t GroupZ;
    };

    struct MaterialConstantData
    {
        uint32_t MaterialIndex;
    };

    struct CameraData
    {
        DXM::Matrix ViewMatrix;
        DXM::Matrix ViewProjectionMatrix;
        Component::Camera::BoundingFrustum Frustum;
    };

    struct MeshCullConstantsData
    {
        uint32_t MeshInstanceCount;
        uint32_t Pad1;
        uint32_t Pad2;
        uint32_t Pad3;
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