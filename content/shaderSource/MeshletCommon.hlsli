#include "DefaultBindings.hlsli"

struct VertexPosition
{
    float3 Position;
};

struct GenericVertexData
{
    float2 UV;
    float3 Normal;
    float3 Tangent;
};

struct VertexOut
{
    float4 Position : SV_Position;
    float2 UV : UV;
#if !NORMAL_MAP
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT; // TODO: Maybe calc it in mesh shader instead of storing it in the vertex data?
    float3x3 TBN : TBN;
#endif
    min16uint MaterialID : Material;
};

struct Payload
{
    uint InstanceID;
    uint MeshletIndex;
};

struct Mesh
{
    uint MeshletCount;
    uint MeshletBuffer;
    uint PrimitiveBuffer;
    uint IndicesBuffer;
    uint PositionBuffer;
    uint VertexDataBuffer;
    float4 Bounds;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
    BoundingSphere Bounds;
};

struct InstanceData
{
    float4x4 Transform;
    uint BatchID;
};

struct BindingConstants
{
    uint InstanceBuffer;
    uint MeshBuffer;
    uint CameraBuffer;
    uint InstanceID;
    uint CameraID;
};

void UnpackBatchID(uint BatchID, out min16uint MeshIndex, out min16uint MaterialIndex)
{
    MeshIndex = (min16uint) (BatchID & 0xFFFF);
    MaterialIndex = (min16uint) ((BatchID >> 16) & 0xFFFF);
}