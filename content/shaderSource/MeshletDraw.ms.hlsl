#include "MeshletCommon.hlsli"
#include "MeshDrawPassCommon.hlsli"

ConstantBuffer<BindingConstants> Bindings : register(b0);

uint3 GetPrimitive(uint primOffset, uint localIndex, StructuredBuffer<uint> primitives)
{
    const uint primitive = primitives[primOffset + localIndex];
    uint ch1, ch2, ch3;
    ch1 = (primitive & 0x000000ff);
    ch2 = (primitive & 0x0000ff00) >> 8;
    ch3 = (primitive & 0x00ff0000) >> 16;
    return uint3(ch1, ch2, ch3);
}

uint GetVertexIndex(uint vertOffset, uint localIndex, StructuredBuffer<uint> indices)
{
    localIndex = vertOffset + localIndex;
    return indices[localIndex];
}

VertexOut GetVertex(uint vertexIndex, float4x4 vpMatrix, StructuredBuffer<VertexPosition> positions, StructuredBuffer<GenericVertexData> genericVertexData, float4x4 transform, min16uint materialIndex)
{
    VertexOut output;
    float4 position = mul(transform, float4(positions[vertexIndex].Position, 1.f));
    output.WorldPosition = position.xyz;
    position = mul(vpMatrix, position);
    output.Position = position;
    
#if !NORMAL_MAP
    const float3 normal = normalize(mul(transform, float4(genericVertexData[vertexIndex].Normal, 0.f))).xyz;
    float3 tangent = normalize(mul(transform, float4(genericVertexData[vertexIndex].Tangent, 0.f))).xyz;
    
    // Re-ortogonalize the tangent since they might not be ortogonal anymore after transform and precision changes. 
    // n * dot(n, t) creates a vector which when you subtract from the tangent creates the new ortogonalized tangent. So its like the "error" vector.
    tangent = tangent - normal * dot(normal, tangent);
    tangent = normalize(tangent);
    
    output.Normal = normal;
    
    const float3 biTangent = normalize(cross(normal, tangent));
    output.TBN = float3x3(tangent, biTangent, normal);
#endif
    
    output.UV = genericVertexData[vertexIndex].UV;
    output.MaterialID = materialIndex;

    return output;
}

float3 HashColor(uint id)
{
    // Simple hash → color
    float r = frac(sin(id * 12.9898) * 43758.5453);
    float g = frac(sin(id * 78.233) * 43758.5453);
    float b = frac(sin(id * 39.3467) * 43758.5453);
    return float3(r, g, b);
}

[NumThreads(88, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    uint dtid : SV_DispatchThreadID,
    out vertices VertexOut verts[64],
    out indices uint3 tris[84]
)
{
    StructuredBuffer<MeshletCulling_To_MeshShader_Data> meshletInstanceBuffer = ResourceDescriptorHeap[Bindings.MeshletInstanceBuffer];
    
    MeshletCulling_To_MeshShader_Data meshletInfo = meshletInstanceBuffer[gid];
    
    min16uint meshIndex, materialIndex;
    UnpackBatchID(meshletInfo.BatchID, meshIndex, materialIndex);
    
    SetMeshOutputCounts(meshletInfo.VertCount, meshletInfo.PrimCount);
    
    const StructuredBuffer<VertexPosition> vertexPositionBuffer = ResourceDescriptorHeap[meshletInfo.PositionBuffer];
    const StructuredBuffer<GenericVertexData> genericVertexDataBuffer = ResourceDescriptorHeap[meshletInfo.VertexDataBuffer];
    const StructuredBuffer<uint> indicesBuffer = ResourceDescriptorHeap[meshletInfo.IndicesBuffer];
    const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[meshletInfo.PrimitiveBuffer];
   
    const StructuredBuffer<CameraData> CameraBuffer = ResourceDescriptorHeap[Bindings.CameraBuffer];
    const CameraData camera = CameraBuffer[Bindings.CameraID];
    const float4x4 vpMatrix = mul(camera.Projection, camera.View);
    
    if (gtid < meshletInfo.PrimCount)
    {
        tris[gtid] = GetPrimitive(meshletInfo.PrimOffset, gtid, primitiveBuffer);
    }
    
    if (gtid < meshletInfo.VertCount)
    {
        uint vertexIndex = GetVertexIndex(meshletInfo.VertOffset, gtid, indicesBuffer);
        verts[gtid] = GetVertex(vertexIndex, vpMatrix, vertexPositionBuffer, genericVertexDataBuffer, meshletInfo.Transform, materialIndex);
        
        // TODO: Make setting
        //float3 meshletColor = HashColor(meshletInfo.LocalMeshletIndex);
        verts[gtid].MeshletColor = float3(1, 1, 1);
    }
}