#include "SceneRenderCommon.hlsli"

struct ConstantData
{
    float4x4 CameraVP;
};

ConstantBuffer<ConstantData> Constants : register(b0);
StructuredBuffer<MeshInstance> MeshInstances : register(t0);
StructuredBuffer<MeshletDrawInstance> MeshletDrawInstances : register(t1);

[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out vertices PipelineVertex verts[63],
    out indices uint3 tris[21]
)
{
    const MeshletDrawInstance meshletInstance = MeshletDrawInstances[gid];
    
    SetMeshOutputCounts(meshletInstance.TriangleCount * 3, meshletInstance.TriangleCount);
    
    if (gtid < meshletInstance.TriangleCount)
    {
        uint index = gtid * 3;
        tris[gtid] = uint3(index, index + 1, index + 2);
    }
    
    if (gtid < meshletInstance.TriangleCount * 3)
    {
        const MeshInstance meshInstance = MeshInstances[meshletInstance.MeshInstanceIndex];
        const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[meshInstance.MeshletBuffer_Bindless + 1]; // The descriptors for the meshlet and vertex buffers are fetched n and n+1
        PipelineVertex newVertex;
        GetVertex(newVertex, meshletInstance.VertexOffset + gtid, Constants.CameraVP, vertexPositionBuffer, meshInstance.WorldTransform, meshInstance.MaterialIndex);
        verts[gtid] = newVertex;
    }
}