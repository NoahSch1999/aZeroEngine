#include "SceneRenderCommon.hlsli"
#include "Util.hlsli"

struct ConstantData
{
    float4x4 CameraVP;
};

//ConstantBuffer<MeshletDrawConstantsData> MeshletDrawConstants : register(b0); // Passed from MeshCull compute shader pass
ConstantBuffer<ConstantData> ConstantsMS : register(b1);

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint localThreadIndex : SV_GroupIndex, // One per vertex in the meshlet
    uint meshletIndex : SV_GroupID,                    // One per meshlet
    in payload MeshletPayload payload,
    out vertices PipelineVertex verts[64],
    out indices uint3 tris[126]
)
{
    if (payload.VisibleMeshletsCount > meshletIndex)
    {
        SetMeshOutputCounts(payload.VertexCount[meshletIndex], payload.VertexCount[meshletIndex]);
    
        if (localThreadIndex < payload.VertexCount[meshletIndex] * 3)
        {
            const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[payload.MeshBuffer_Bindless + 1];
            tris[localThreadIndex] = Unpack32To8(primitiveBuffer[payload.VertexOffset[localThreadIndex] + localThreadIndex]);
        }
    
        if (localThreadIndex < payload.VertexCount[meshletIndex])
        {
            PipelineVertex newVertex;
            const StructuredBuffer<MeshVertex> vertexPositionBuffer = ResourceDescriptorHeap[payload.MeshBuffer_Bindless + 2];
            GetVertex(newVertex, payload.VertexOffset[meshletIndex] + localThreadIndex, ConstantsMS.CameraVP, vertexPositionBuffer, payload.WorldTransform);
            verts[localThreadIndex] = newVertex;
        }
    }
}