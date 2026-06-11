#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "ecs/Components.hpp"

namespace aZero::Rendering::GPU_Struct
{
    struct CameraData
    {
        DXM::Matrix ViewMatrix;
        DXM::Matrix ViewProjectionMatrix;
        Component::Camera::BoundingFrustum Frustum;
    };

    // Object cull
    struct IndirectArgumentCounter
    {
        uint32_t Count;
    };

    /*
    Per-object
    Updated on entity update
    */
    struct InstanceData
    {
        DXM::Matrix Transform;
    };

    struct IndirectArgumentConstantData
    {
        uint32_t InstanceIndex;
        uint32_t GlobalMeshletOffset;
        uint32_t GlobalVertexOffset;
        uint32_t MaterialIndex;
    };

    /*
    Per-object
    Updated on entity update
    Used in the object culling CS to check object-visibility.
    */
    struct ObjectCullData
    {
        DirectX::BoundingSphere Bounds;
        uint32_t GlobalMeshletOffset;
        uint32_t GlobalVertexOffset;
        uint32_t MaterialIndex;
    };

    struct MeshCullConstantsData
    {
        uint32_t MeshInstanceCount;
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
    //

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