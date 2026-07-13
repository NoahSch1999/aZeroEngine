#include "GPU_Structs.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

ConstantBuffer<CameraData> CameraDataBuffer : register(b1);

StructuredBuffer<Meshlet> MeshletBuffer : register(t0);
StructuredBuffer<MeshVertex> VertexBuffer : register(t1);

[NumThreads(g_PrimitivesPerMeshlet, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint localThreadIndex : SV_GroupIndex,
    uint meshletIndex : SV_GroupID,
    in payload Payload payload,
    out vertices RasterVertex verts[g_VerticesPerMeshlet],
    out indices uint3 tris[g_PrimitivesPerMeshlet]
)
{
    if (Input_CONSTANT.MeshletCount > meshletIndex)
    {
        const uint vertexCount = MeshletBuffer[payload.MeshletIndex[meshletIndex]].VertexCount;
        {
            const uint primitiveCount = MeshletBuffer[payload.MeshletIndex[meshletIndex]].PrimitiveCount;
            
            SetMeshOutputCounts(vertexCount, primitiveCount);
        
            if (localThreadIndex < primitiveCount)
            {
                tris[localThreadIndex] = Unpack32To8(MeshletBuffer[payload.MeshletIndex[meshletIndex]].Primitives[localThreadIndex]);
            }
        }
    
        if (localThreadIndex < vertexCount)
        {
            MeshVertex vertex = VertexBuffer[Input_CONSTANT.GlobalVertexOffset + MeshletBuffer[payload.MeshletIndex[meshletIndex]].VertexOffset + localThreadIndex];
            float4 positionWorld = mul(Input_CONSTANT.Instance, float4(vertex.Position.xyz, 1.f));
            verts[localThreadIndex].WorldPosition = positionWorld.xyz;
            positionWorld = mul(CameraDataBuffer.ViewProjectionMatrix, positionWorld);
            verts[localThreadIndex].Position = positionWorld;
            verts[localThreadIndex].Normal = normalize(mul(Input_CONSTANT.Instance, float4(DecodeNormalOctahedral(UnpackOct16(Unpack32To16(vertex.Normal))), 0.f))).xyz;
            verts[localThreadIndex].UV = Unpack32ToHalfFloats(vertex.UV);
            verts[localThreadIndex].Meshletid = HashColor(meshletIndex);
        }
    }
}