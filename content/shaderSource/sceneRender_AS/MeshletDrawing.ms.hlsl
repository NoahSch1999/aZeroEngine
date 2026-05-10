#include "SceneRenderCommon.hlsli"

// Root constants passed via indirect draw
/*
    struct From_CS_ConstantsData
    {
        uint MeshInstanceIndex; // Mesh instance index to look into MeshInstance_Bindless
        uint MaterialIndex; // Material instance
    };
*/
ConstantBuffer<From_CS_ConstantsData> From_CS_Constants : register(b0);

// Set from the cpu like normal
struct PassConstantData
{
    uint MeshInstance_Bindless; // Bindless index to the mesh instances buffer
    float4x4 CameraVP; // Camera view + projection matrix
};
ConstantBuffer<PassConstantData> PassConstants : register(b1);

[NumThreads(THREADS_PER_MESHLET, 1, 1)] // This assumes that the vertex count of each meshlet is less or equal to THREADS_PER_MESHLET
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload MeshletPayload payload,
    out vertices PipelineVertex verts[64],
    out indices uint3 tris[THREADS_PER_MESHLET - 4]
)
{
    SetMeshOutputCounts(payload.data[gid].VertexCount, payload.data[gid].VertexCount * 3);
    
    if (gtid < payload.data[gid].VertexCount * 3) // Prims will always be vertex count * 3 since we are rendering triangles. Maybe take a look at this if we want to draw wireframes...
    {
        tris[gtid] = uint3(gtid, gtid + 1, gtid + 2); // This assumes that all meshlets store their vertices of every single triangle as [n, n+1, n+2] where n=payload.data[gid].VertexOffset
    }
    
    if (gtid < payload.data[gid].VertexCount)
    {
        StructuredBuffer<MeshInstance> meshInstanceBuffer = ResourceDescriptorHeap[PassConstants.MeshInstance_Bindless];
        const MeshInstance meshInstance = meshInstanceBuffer[From_CS_Constants.MeshInstanceIndex];
        const StructuredBuffer<PipelineVertex> vertexBuffer = ResourceDescriptorHeap[meshInstance.VertexBuffer_Bindless];
        MeshVertex inVertex = vertexBuffer[payload.data[gid].VertexOffset + gtid];
        
        PipelineVertex outVertex;
        outVertex.Position = mul(meshInstance.WorldTransform, float4(inVertex.Position, 1.f));
        outVertex.WorldPosition = outVertex.Position.xyz;
        outVertex.Position = mul(PassConstants.CameraVP, outVertex.Position);
        outVertex.UV = inVertex.UV;
        outVertex.Normal = normalize(mul(meshInstance.WorldTransform, float4(inVertex.Normal, 0.f))).xyz;

#if !NORMAL_MAP
        float3 tangent = normalize(mul(meshInstance.WorldTransform, float4(inVertex.Tangent, 0.f))).xyz;

        // Re-ortogonalize the tangent since they might not be ortogonal anymore after transform and precision changes. 
        // n * dot(n, t) creates a vector which when you subtract from the tangent creates the new ortogonalized tangent. So its like the "error" vector.
        tangent = tangent - outVertex.Normal * dot(outVertex.Normal, tangent);
        tangent = normalize(tangent);

        const float3 biTangent = normalize(cross(outVertex.Normal, tangent));
        outVertex.TBN = float3x3(tangent, biTangent, outVertex.Normal);
#endif
        verts[gtid] = outVertex;
    }
}