#include "MeshletCommon.hlsli"

ConstantBuffer<BindingConstants> Bindings : register(b0);

uint3 GetPrimitive(Meshlet meshlet, uint index, StructuredBuffer <uint> primitives)
{
    const uint primitive = primitives[meshlet.PrimOffset + index];
    uint ch1, ch2, ch3, ch4;
    ch1 = primitive >> 24;
    ch2 = (primitive & 0x00ff0000) >> 16;
    ch3 = (primitive & 0x0000ff00) >> 8;
    ch4 = (primitive & 0x000000ff);
    return uint3(ch1, ch2, ch3);
}

uint GetVertexIndex(Meshlet meshlet, uint localIndex, StructuredBuffer<uint> indices)
{
    localIndex = meshlet.VertOffset + localIndex;
    return indices[localIndex];
}

VertexOut GetVertex(uint vertexIndex, float4x4 vpMatrix, StructuredBuffer<VertexPosition> positions, StructuredBuffer<GenericVertexData> genericVertexData, float4x4 transform, min16uint materialIndex)
{
    VertexOut output;
    float4 position = mul(transform, float4(positions[vertexIndex].Position, 1.f));
    position = mul(vpMatrix, position);
    output.Position = position;
    
#if !NORMAL_MAP
    const float3 normal = mul(transform, float4(genericVertexData[vertexIndex].Normal, 0.f)).xyz;
    float3 tangent = mul(transform, float4(genericVertexData[vertexIndex].Tangent, 0.f)).xyz;
    
    // Re-ortogonalize the tangent since they might not be ortogonal anymore after transform and precision changes. 
    // n * dot(n, t) creates a vector which when you subtract from the tangent creates the new ortogonalized tangent. So its like the "error" vector.
    tangent = tangent - normal * dot(normal, tangent);
    tangent = normalize(tangent);
    
    output.Normal = normal;
    output.Tangent = tangent;
    
    const float3 biTangent = normalize(cross(tangent, normal));
    output.TBN = float3x3(tangent, biTangent, normal);
#endif
    
    output.UV = genericVertexData[vertexIndex].UV;
    output.MaterialID = materialIndex;

    return output;
}

[NumThreads(126, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out vertices VertexOut verts[64],
    out indices uint3 tris[126]
)
{
    const StructuredBuffer<InstanceData> instances = ResourceDescriptorHeap[Bindings.InstanceBuffer];
    const InstanceData instance = instances[Bindings.InstanceID];
    
    min16uint meshIndex, materialIndex;
    UnpackBatchID(instance.BatchID, meshIndex, materialIndex);
    
    const StructuredBuffer<Mesh> meshes = ResourceDescriptorHeap[Bindings.MeshBuffer];
    const Mesh mesh = meshes[meshIndex];
    
    const StructuredBuffer<Meshlet> meshlets = ResourceDescriptorHeap[mesh.MeshletBuffer];
    const Meshlet meshlet = meshlets[payload.MeshletIndex];
    
    const StructuredBuffer<VertexPosition> vertexPositionBuffer = ResourceDescriptorHeap[mesh.PositionBuffer];
    const StructuredBuffer<GenericVertexData> genericVertexDataBuffer = ResourceDescriptorHeap[mesh.VertexDataBuffer];
    const StructuredBuffer<uint> indicesBuffer = ResourceDescriptorHeap[mesh.IndicesBuffer];
    const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[mesh.PrimitiveBuffer];
   
    const StructuredBuffer<CameraData> CameraBuffer = ResourceDescriptorHeap[Bindings.CameraBuffer];
    const CameraData camera = CameraBuffer[Bindings.CameraID];
    const float4x4 vpMatrix = mul(camera.View, camera.Projection);
    
    SetMeshOutputCounts(meshlet.VertCount, meshlet.PrimCount);
    
    if (gtid < meshlet.PrimCount)
    {
        tris[gtid] = GetPrimitive(meshlet, gtid, primitiveBuffer);
    }
    
    if (gtid < meshlet.VertCount)
    {
        verts[gtid] = GetVertex(GetVertexIndex(meshlet, gtid, indicesBuffer), vpMatrix, vertexPositionBuffer, genericVertexDataBuffer, instance.Transform, materialIndex);
    }
}