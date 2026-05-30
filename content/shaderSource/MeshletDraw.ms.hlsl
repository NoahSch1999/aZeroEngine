#include "MeshDefinitions.hlsli"
#include "Util.hlsli"
#include "PayloadDefinitions.hlsli"
#include "GeometryPipeline_IA.hlsli"
#include "Camera.hlsli"

ConstantBuffer<IndirectArgumentConstantData> Input_CONSTANT : register(b0);

ConstantBuffer<Camera> CameraBuffer : register(b1);

StructuredBuffer<MeshInstanceData> MeshInstances : register(t0);

[NumThreads(88, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint localThreadIndex : SV_GroupIndex,
    uint meshletIndex : SV_GroupID,
    in payload Payload payload,
    out vertices RasterVertex verts[64],
    out indices uint3 tris[84]
)
{
    const MeshInstanceData meshInstance = MeshInstances[Input_CONSTANT.MeshInstanceIndex.x];
    if (meshInstance.Instance.MeshletCount > meshletIndex)
    {
        const StructuredBuffer<Meshlet> Meshlets = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex];
        const Meshlet meshlet = Meshlets[payload.MeshletIndex[meshletIndex]]; // Top stall
        SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);
        
        if (localThreadIndex < meshlet.PrimitiveCount)
        {
            const StructuredBuffer<uint> primitiveBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 1];
            tris[localThreadIndex] = Unpack32To8(primitiveBuffer[meshlet.PrimitiveOffset + localThreadIndex]);
        }
    
        if (localThreadIndex < meshlet.VertexCount)
        {
            const StructuredBuffer<uint> indices = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 3];
            const uint index = indices[meshlet.VertexOffset + localThreadIndex]; // Top stall
            
            const StructuredBuffer<float3> vertexPositionBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 4];
            float3 position = vertexPositionBuffer[index];
            
            float4 positionWorld = mul(meshInstance.Instance.WorldTransform, float4(position, 1.f));
            verts[localThreadIndex].WorldPosition = positionWorld.xyz;
            positionWorld = mul(CameraBuffer.ViewProjectionMatrix, positionWorld);
            verts[localThreadIndex].Position = positionWorld;
            
            const StructuredBuffer<MeshVertex> vertexBuffer = ResourceDescriptorHeap[meshInstance.Instance.MeshBufferIndex + 2];
            
            MeshVertex vertex = vertexBuffer[index];
            verts[localThreadIndex].Normal = normalize(mul(meshInstance.Instance.WorldTransform, float4(DecodeNormalOctahedral(UnpackOct16(Unpack32To16(vertex.Normal))), 0.f))).xyz;
            verts[localThreadIndex].UV = UnpackUV16(Unpack32To16(vertex.UV));
        }
    }
}